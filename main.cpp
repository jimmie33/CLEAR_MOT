#include <iostream>
#include <sstream>
#include <fstream>
#include <time.h>

#include "libxml/parser.h"
#include "libxml/tree.h"
#include "libxml/xmlmemory.h"

#include "clearMOT.h"
#include "dataReader.h"

#define GT_XML_FILE "TownCentre_GT.xml"
#define HP_XML_FILE "temp.xml"

int calMOT()
{
	ResultParser gt(GT_XML_FILE,1.0);
	ResultParser hp(HP_XML_FILE,1.0,		1.0,			0.8);//1.0*1.0*w,1.0*0.8*h
	//                          [ratio]		[width_ratio]	[height_ratio]
	C_Mot mot;
	
	while(!gt.isEnd() && !hp.isEnd())
	{
		mot.dealWith(gt.readNextFrame(),hp.readNextFrame());
	}
	mot.getMOT();
	getchar();
	return 1;
}

int main()
{
	calMOT();
	return 0;
}