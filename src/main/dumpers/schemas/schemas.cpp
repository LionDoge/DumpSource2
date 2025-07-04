/**
 * =============================================================================
 * DumpSource2
 * Copyright (C) 2024 ValveResourceFormat Contributors
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "schemas.h"
#include "globalvariables.h"
#include "interfaces.h"
#include "schemasystem/schemasystem.h"
#include <filesystem>
#include <fstream>
#include <map>
#include <unordered_set>
#include <algorithm>
#include "metadatalist.h"
#include <optional>
#include <cstring>
#include <fmt/format.h>

namespace Dumpers::Schemas
{
// Determine how and if to output metadata entry value based on it's type.
std::optional<std::string> GetMetadataValue(const SchemaMetadataEntryData_t& entry)
{
	const auto hashedName = hash_32_fnv1a_const(entry.m_pszName);
	if (std::ranges::find(string_metadata_entries, hashedName) != string_metadata_entries.end()) {
		return fmt::format("\"{}\"", *static_cast<const char**>(entry.m_pData));
	}
	else if (std::ranges::find(integer_metadata_entries, hashedName) != integer_metadata_entries.end()) {
		int result;
		std::memcpy(&result, entry.m_pData, sizeof(int));
		return std::to_string(result);
	}
	else if (std::ranges::find(float_metadata_entries, hashedName) != float_metadata_entries.end()) {
		float result;
		std::memcpy(&result, entry.m_pData, sizeof(float));
		return std::to_string(result);
	}
	else if (std::ranges::find(inline_string_metadata_entries, hashedName) != inline_string_metadata_entries.end()) {
		// max 8 characters. Also check for null term.
		char* result = static_cast<char*>(entry.m_pData);
		for (uint8_t i = 0; i < 8; ++i) {
			if (result[i] == '\0') {
				return fmt::format("\"{}\"", std::string(result, i));
			}
		}
		return fmt::format("\"{}\"", std::string(result, 8));
	}
	return {};
}

void OutputMetadataEntry(const SchemaMetadataEntryData_t& entry, std::ofstream& output, bool tabulate)
{
	output << (tabulate ? "\t" : "") << "// " << entry.m_pszName;
	const auto metadataValue = GetMetadataValue(entry);
	if (metadataValue)
		output << " = " << *metadataValue;

	output << "\n";
}

void DumpClasses(CSchemaSystemTypeScope* typeScope, std::filesystem::path schemaPath, std::map<std::string, std::unordered_set<std::string>>& foundFiles)
{
	const auto& classes = typeScope->m_ClassBindings;

	UtlTSHashHandle_t* handles = new UtlTSHashHandle_t[classes.Count()];
	classes.GetElements(0, classes.Count(), handles);

	for (int j = 0; j < classes.Count(); ++j) {
		const auto classInfo = classes[handles[j]];

		if (!std::filesystem::is_directory(schemaPath / classInfo->m_pszProjectName))
			if (!std::filesystem::create_directory(schemaPath / classInfo->m_pszProjectName))
				return;

		// Some classes have :: in them which we can't save.
		auto sanitizedFileName = std::string(classInfo->m_pszName);
		std::replace(sanitizedFileName.begin(), sanitizedFileName.end(), ':', '_');

		// We save the file in a map so that we know which files are outdated and should be removed
		foundFiles[classInfo->m_pszProjectName].insert(sanitizedFileName);

		std::ofstream output((schemaPath / classInfo->m_pszProjectName / sanitizedFileName).replace_extension(".h"));

		// Output metadata entries as comments before the class definition
		for (uint16_t k = 0; k < classInfo->m_nStaticMetadataCount; k++)
		{
			const auto& metadataEntry = classInfo->m_pStaticMetadata[k];
			OutputMetadataEntry(metadataEntry, output, false);
		}

		output << "class " << classInfo->m_pszName;
		Globals::stringsIgnoreStream << classInfo->m_pszName << "\n";

		if (classInfo->m_nBaseClassCount > 0)
			output << " : public " << classInfo->m_pBaseClasses[0].m_pClass->m_pszName;

		output << "\n{\n";

		for (uint16_t k = 0; k < classInfo->m_nFieldCount; k++)
		{
			const auto& field = classInfo->m_pFields[k];
			// Output metadata entires as comments before the field definition
			for (uint16_t l = 0; l < field.m_nStaticMetadataCount; l++)
			{
				const auto& metadataEntry = field.m_pStaticMetadata[l];
				OutputMetadataEntry(metadataEntry, output, true);
			}

			output << "\t" << field.m_pType->m_sTypeName.String() << " " << field.m_pszName << ";\n";
			Globals::stringsIgnoreStream << field.m_pszName << "\n";
		}

		output << "};\n";
	}
}

void DumpEnums(CSchemaSystemTypeScope* typeScope, std::filesystem::path schemaPath, std::map<std::string, std::unordered_set<std::string>>& foundFiles)
{
	const auto& enums = typeScope->m_EnumBindings;

	UtlTSHashHandle_t* handles = new UtlTSHashHandle_t[enums.Count()];
	enums.GetElements(0, enums.Count(), handles);

	for (int j = 0; j < enums.Count(); ++j) {
		const auto enumInfo = enums[handles[j]];

		if (!std::filesystem::is_directory(schemaPath / enumInfo->m_pszProjectName))
			if (!std::filesystem::create_directory(schemaPath / enumInfo->m_pszProjectName))
				return;

		// Some classes have :: in them which we can't save.
		auto sanitizedFileName = std::string(enumInfo->m_pszName);
		std::replace(sanitizedFileName.begin(), sanitizedFileName.end(), ':', '_');

		// We save the file in a map so that we know which files are outdated and should be removed
		foundFiles[enumInfo->m_pszProjectName].insert(sanitizedFileName);

		std::ofstream output((schemaPath / enumInfo->m_pszProjectName / sanitizedFileName).replace_extension(".h"));

		for (uint16_t k = 0; k < enumInfo->m_nStaticMetadataCount; k++)
		{
			const auto& metadataEntry = enumInfo->m_pStaticMetadata[k];
			OutputMetadataEntry(metadataEntry, output, false);
		}

		output << "enum " << enumInfo->m_pszName << " : ";
		Globals::stringsIgnoreStream << enumInfo->m_pszName << "\n";

		switch (enumInfo->m_nAlignment)
		{
		case 1:
			output << "uint8_t";
			break;
		case 2:
			output << "uint16_t";
			break;
		case 4:
			output << "uint32_t";
			break;
		case 8:
			output << "uint64_t";
			break;
		default:
			output << "unknown alignment type";
		}

		output << "\n{\n";

		for (uint16_t k = 0; k < enumInfo->m_nEnumeratorCount; k++)
		{
			const auto& field = enumInfo->m_pEnumerators[k];
			// Output metadata entires as comments before the field definition
			for (uint16_t l = 0; l < field.m_nStaticMetadataCount; l++)
			{
				const auto& metadataEntry = field.m_pStaticMetadata[l];
				OutputMetadataEntry(metadataEntry, output, true);
			}

			output << "\t" << field.m_pszName << " = " << field.m_nValue << ",\n";
			Globals::stringsIgnoreStream << field.m_pszName << "\n";
		}

		output << "};\n";
	}
}


void DumpTypeScope(CSchemaSystemTypeScope* typeScope, std::filesystem::path schemaPath, std::map<std::string, std::unordered_set<std::string>>& foundFiles)
{
	DumpClasses(typeScope, schemaPath, foundFiles);
	DumpEnums(typeScope, schemaPath, foundFiles);
}

void Dump()
{
	auto schemaSystem = Interfaces::schemaSystem;

	const auto& typeScopes = schemaSystem->m_TypeScopes;
	const auto schemaPath = Globals::outputPath / "schemas";

	if (!std::filesystem::is_directory(schemaPath))
		if (!std::filesystem::create_directory(schemaPath))
			return;

	std::map<std::string, std::unordered_set<std::string>> foundFiles;

	for (auto i = 0; i < typeScopes.GetNumStrings(); ++i)
		DumpTypeScope(typeScopes[i], schemaPath, foundFiles);

	DumpTypeScope(schemaSystem->GlobalTypeScope(), schemaPath, foundFiles);

	for (const auto& entry : std::filesystem::directory_iterator(schemaPath))
	{
		auto projectName = entry.path().filename().string();
		bool isInMap = foundFiles.find(projectName) != foundFiles.end();

		if (entry.is_directory() && !isInMap)
		{
			printf("Removing %s\n", entry.path().generic_string().c_str());
			std::filesystem::remove_all(entry.path());
		}
		else if (isInMap)
		{

			for (const auto& typeScopePath : std::filesystem::directory_iterator(entry.path()))
			{
				auto& filesSet = foundFiles[projectName];

				if (filesSet.find(typeScopePath.path().stem().string()) == filesSet.end())
				{
					printf("Removing %s\n", typeScopePath.path().generic_string().c_str());
					std::filesystem::remove(typeScopePath.path());
				}
			}
		}
	}
}

} // namespace Dumpers::Schemas