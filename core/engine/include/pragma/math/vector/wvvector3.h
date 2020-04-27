/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2020 Florian Weischer
 */

#ifndef __WVVECTOR3_H__
#define __WVVECTOR3_H__
#include "pragma/definitions.h"
#include <mathutil/uvec.h>
#include "pragma/console/conout.h"

DLLENGINE Con::c_cout& operator<<(Con::c_cout &os,const Vector4 &vec);
DLLENGINE Con::c_cout& operator<<(Con::c_cout &os,const Vector3 &vec);
DLLENGINE Con::c_cout& operator<<(Con::c_cout &os,const Vector2 &vec);

namespace uvec
{
	DLLENGINE void print(Vector3 *vec);
};

#endif // __VECTOR3_H__