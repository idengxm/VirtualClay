// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <Leap.h>
#include <LeapMath.h>
#include "Mudbox\mudbox.h"
#include <vector>
#include "util.h"
#include "ID_List.h"
#define mblog(a) mudbox::Kernel()->Log(a);
//
//#define max(a,b) (a > b) ? a : b
//#define min(a,b) (a < b) ? a : b

class VirtualClay: mudbox::Node {
  DECLARE_CLASS
    Q_DECLARE_TR_FUNCTIONS(VirtualClay);
public:
	static void Initializer(void);
	static void Execute(void);   // Execute 
	
};










// TODO: reference additional headers your program requires here