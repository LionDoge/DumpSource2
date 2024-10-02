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

#include "concommands.h"
#include "interfaces.h"
#include "icvar.h"

namespace Dumpers::ConCommands
{

void Dump()
{

	ConVarHandle hCvarHandle;
	hCvarHandle.Set(0);
	ConVar* pCvar = nullptr;

	do
	{
		pCvar = Interfaces::cvar->GetConVar(hCvarHandle);

		hCvarHandle.Set(hCvarHandle.Get() + 1);

		if (!pCvar)
			continue;

		printf("name: %s\n", pCvar->m_pszName);

	} while (pCvar);
}

} // namespace Dumpers::ConCommands