/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#ifndef _MODEL_JOE03_H
#define _MODEL_JOE03_H

#include "model.h"
#include <iosfwd>

class JOEPACK;
struct JOEObject;

// This class handles all of the loading code
class MODEL_JOE03 : public MODEL
{
public:
	virtual ~MODEL_JOE03()
	{
		Clear();
	}

	virtual bool Load(const std::string & strFileName, std::ostream & error_output, bool genlist)
	{
		return Load(strFileName, error_output, genlist, 0);
	}

	virtual bool CanSave() const
	{
		return false;
	}

	bool Load(const std::string & strFileName, std::ostream & error_output, bool genlist, const JOEPACK * pack);



private:
	static const int JOE_MAX_FACES;
	static const int JOE_VERSION;
	static const float MODEL_SCALE;

	// This reads in the data from the MD2 file and stores it in the member variable
	void ReadData(FILE * m_FilePointer, const JOEPACK * pack, JOEObject & Object);

	bool LoadFromHandle(FILE * f, const JOEPACK * pack, std::ostream & error_output);
};

#endif
