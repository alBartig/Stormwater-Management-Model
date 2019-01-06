/** @file toolkitAPI.c
 @see http://github.com/openwateranalytics/stormwater-management-model

 toolkitAPI.c
 @brief Exportable Functions for Toolkit API.
 @date 08/30/2016 (First Contribution)
 @authors B. McDonnell (EmNet LLC), OpenWaterAnalytics members: see <a href="https://github.com/OpenWaterAnalytics/Stormwater-Management-Model/blob/develop/AUTHORS">AUTHORS</a>.


*/
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "headers.h"
#include "swmm5.h"                     // declaration of exportable functions
#include "toolkitAPI.h"
#include "hash.h"

// Function Declarations for API
int     massbal_getRoutingFlowTotal(SM_RoutingTotals *routingTot);
int     massbal_getRunoffTotal(SM_RunoffTotals *runoffTot);
double  massbal_getTotalArea(void);
int     massbal_getNodeTotalInflow(int index, double *value);

int  stats_getNodeStat(int index, SM_NodeStats *nodeStats);
int  stats_getStorageStat(int index, SM_StorageStats *storageStats);
int  stats_getOutfallStat(int index, SM_OutfallStats *outfallStats);
int  stats_getLinkStat(int index, SM_LinkStats *linkStats);
int  stats_getPumpStat(int index, SM_PumpStats *pumpStats);
TSubcatchStats *stats_getSubcatchStat(int index);

// Utilty Function Declarations
double* newDoubleArray(int n);

//-----------------------------------------------------------------------------
//  Extended API Functions
//-----------------------------------------------------------------------------
void DLLEXPORT swmm_getAPIError(int errcode, char *s)
///
/// Input:   errcode = error code
/// Output:  errmessage String
/// Return:  API Error
/// Purpose: Get an error message
{
    char *errmsg = error_getMsg(errcode);
    strcpy(s, errmsg);
}


int DLLEXPORT swmm_project_findObject(int type, char *id, int *index)
{
    int errorcode = 0;

	int idx = project_findObject(type, id);

	if (idx == -1) {
        index    = NULL;
        errorcode = ERR_API_OBJECT_INDEX;
	} else
        *index = idx;

	return errorcode;
}

int DLLEXPORT swmm_getSimulationDateTime(int timetype, int *year, int *month, int *day,
                                         int *hour, int *minute, int *second)
///
/// Input:   timetype = time type to return
/// Output:  year, month, day, hours, minutes, seconds = int
/// Return:  API Error
/// Purpose: Get the simulation start, end and report date times
{
    int errcode = 0;
    *year = 1900;
    *month = 1;
    *day = 1;
    *hour = 0;
    *minute = 0;
    *second = 0;

    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    else
    {
        DateTime _dtime;
        switch (timetype)
        {
        //StartDateTime (globals.h)
        case SM_STARTDATE: _dtime = StartDateTime; break;
        //EndDateTime (globals.h)
        case SM_ENDDATE: _dtime = EndDateTime;  break;
        //ReportStart (globals.h)
        case SM_REPORTDATE: _dtime = ReportStart;  break;
        default: return(ERR_API_OUTBOUNDS);
        }
        datetime_decodeDate(_dtime, year, month, day);
        datetime_decodeTime(_dtime, hour, minute, second);
    }

    return (errcode);
}

int DLLEXPORT swmm_setSimulationDateTime(int timetype, char *dtimestr)
///
/// Input:   timetype = time type to return
///          DateTime String
/// Return:  API Error
/// Purpose: Get the simulation start, end and report date times
{
    int errcode = 0;

    char theDate[10];
    char theTime[9];

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if Simulation is Running
    else if(swmm_IsStartedFlag() == TRUE)
    {
        errcode = ERR_API_SIM_NRUNNING;
    }
    else
    {
        strncpy(theDate, dtimestr, 10);
        strncpy(theTime, dtimestr+11, 9);

        switch(timetype)
        {
            //StartDateTime (globals.h)
            case SM_STARTDATE:
                project_readOption("START_DATE", theDate);
                project_readOption("START_TIME", theTime);
                StartDateTime = StartDate + StartTime;
                TotalDuration = floor((EndDateTime - StartDateTime) * SECperDAY);
                // --- convert total duration to milliseconds
                TotalDuration *= 1000.0;
                break;
            //EndDateTime (globals.h)
            case SM_ENDDATE:
                project_readOption("END_DATE", theDate);
                project_readOption("END_TIME", theTime);
                EndDateTime = EndDate + EndTime;
                TotalDuration = floor((EndDateTime - StartDateTime) * SECperDAY);
                // --- convert total duration to milliseconds
                TotalDuration *= 1000.0;
                break;
            //ReportStart (globals.h)
            case SM_REPORTDATE:
                project_readOption("REPORT_START_DATE", theDate);
                project_readOption("REPORT_START_TIME", theTime);
                ReportStart = ReportStartDate + ReportStartTime;
                break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }

    return (errcode);
}

int DLLEXPORT  swmm_getSimulationUnit(int type, int *value)
///
/// Input:   type = simulation unit type
/// Output:  enum representation of units
/// Returns: API Error
/// Purpose: get simulation unit types
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    else
    {
        switch(type)
        {
            // System Unit (enum.h UnitsType)
            case SM_SYSTEMUNIT:  *value = UnitSystem; break;
            // Flow Unit (enum.h FlowUnitsType)
            case SM_FLOWUNIT:  *value = FlowUnits; break;
            // Concentration Unit
            //case 2:  *value = UnitSystem; break;
            // Type not available
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }

    return (errcode);
}

int DLLEXPORT  swmm_getSimulationAnalysisSetting(int type, int *value)
///
/// Input:   type = analysis type
/// Output:  setting True or False
/// Returns: API Error
/// Purpose: get simulation analysis setting
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    else
    {
        switch(type)
        {
            // No ponding at nodes (True or False)
            case SM_ALLOWPOND:  *value = AllowPonding; break;
            // Do flow routing in steady state periods  (True or False)
            case SM_SKIPSTEADY:  *value = SkipSteadyState; break;
            // Analyze rainfall/runoff  (True or False)
            case SM_IGNORERAIN:  *value = IgnoreRainfall; break;
            // Analyze RDII (True or False)
            case SM_IGNORERDII:  *value = IgnoreRDII; break;
            // Analyze snowmelt (True or False)
            case SM_IGNORESNOW:  *value = IgnoreSnowmelt; break;
            // Analyze groundwater (True or False)
            case SM_IGNOREGW:  *value = IgnoreGwater; break;
            // Analyze flow routing (True or False)
            case SM_IGNOREROUTE:  *value = IgnoreRouting; break;
            // Analyze water quality (True or False)
            case SM_IGNORERQUAL:  *value = IgnoreQuality; break;
            // Type not available
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return (errcode);
}

int DLLEXPORT  swmm_getSimulationParam(int type, double *value)
///
/// Input:   type = analysis type
/// Output:  Simulation Parameter
/// Returns: error code
/// Purpose: Get simulation analysis parameter
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Output  setting
    else
    {
        switch(type)
        {
            // Routing time step (sec)
            case SM_ROUTESTEP: *value = RouteStep; break;
            // Minimum variable time step (sec)
            case SM_MINROUTESTEP: *value = MinRouteStep; break;
            // Time step for lengthening (sec)
            case SM_LENGTHSTEP: *value = LengtheningStep; break;
            // Antecedent dry days
            case SM_STARTDRYDAYS: *value = StartDryDays; break;
            // Courant time step factor
            case SM_COURANTFACTOR: *value = CourantFactor; break;
            // Minimum nodal surface area
            case SM_MINSURFAREA: *value = MinSurfArea; break;
            // Minimum conduit slope
            case SM_MINSLOPE: *value = MinSlope; break;
            // Runoff continuity error
            case SM_RUNOFFERROR: *value = RunoffError; break;
            // Groundwater continuity error
            case SM_GWERROR: *value = GwaterError; break;
            // Flow routing error
            case SM_FLOWERROR: *value = FlowError; break;
            // Quality routing error
            case SM_QUALERROR: *value = QualError; break;
            // DW routing head tolerance (ft)
            case SM_HEADTOL: *value = HeadTol; break;
            // Tolerance for steady system flow
            case SM_SYSFLOWTOL: *value = SysFlowTol; break;
            // Tolerance for steady nodal inflow
            case SM_LATFLOWTOL: *value = LatFlowTol; break;
            // Type not available
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return (errcode);
}

int DLLEXPORT  swmm_countObjects(int type, int *count)
///
/// Input:   type = object type (Based on SM_ObjectType enum)
/// Output:  count = pointer to integer
/// Returns: API Error
/// Purpose: uses Object Count table to find number of elements of an object
{
    *count = 0;
    if(type >= MAX_OBJ_TYPES)return ERR_API_OUTBOUNDS;
    *count = Nobjects[type];
    return (0);
}

int DLLEXPORT swmm_getObjectIndex(SM_ObjectType type, char *id, int *index)
///
/// Input:   type = object type (Based on SM_ObjectType enum)
///          char* = ID name
/// Output:  object index
/// Return:  error
/// Purpose: Gets object id index
{
    int error = 0;

    // Check if Open
    if(swmm_IsOpenFlag() == TRUE)
        *index = project_findObject(type, id);
    else
        error = ERR_API_INPUTNOTOPEN;

    return error;
}

int DLLEXPORT swmm_getObjectId(int type, int index, char *id)
///
/// Input:   type = object type (Based on SM_ObjectType enum)
///          index = Index of desired ID
/// Output:  id = pointer to id pass by reference
/// Return:  API Error
/// Purpose: Gets ID for any object
{
    int errcode = 0;
    TLidProc*  lidProc;
    //Provide Empty Character Array
    strcpy(id,"");

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[type])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch (type)
        {
            case SM_GAGE:
                strcpy(id,Gage[index].ID); break;
            case SM_SUBCATCH:
                strcpy(id,Subcatch[index].ID); break;
            case SM_NODE:
                strcpy(id,Node[index].ID); break;
            case SM_LINK:
                strcpy(id,Link[index].ID); break;
            case SM_POLLUT:
                strcpy(id,Pollut[index].ID); break;
            case SM_LANDUSE:
                strcpy(id,Landuse[index].ID); break;
            case SM_TIMEPATTERN:
                strcpy(id,Pattern[index].ID); break;
            case SM_CURVE:
                strcpy(id,Curve[index].ID); break;
            case SM_TSERIES:
                strcpy(id,Tseries[index].ID); break;
            //case SM_CONTROL:
                //strcpy(id,Rules[index].ID); break;
            case SM_TRANSECT:
                strcpy(id,Transect[index].ID); break;
            case SM_AQUIFER:
                strcpy(id,Aquifer[index].ID); break;
            case SM_UNITHYD:
                strcpy(id,UnitHyd[index].ID); break;
            case SM_SNOWMELT:
                strcpy(id,Snowmelt[index].ID); break;
            //case SM_SHAPE:
                //strcpy(id,Shape[index].ID); break;
            case SM_LID:
                lidProc = lid_getLidProc(index);
                if (lidProc != NULL)
                {
                    strcpy(id,lidProc->ID); 
                }
                break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
   }
   return(errcode);
}

int DLLEXPORT swmm_getNodeType(int index, int *Ntype)
///
/// Input:   index = Index of desired ID
///          Ntype = Node type (Based on enum SM_NodeType)
/// Return:  API Error
/// Purpose: Gets Node Type
{
    int errcode = 0;
    *Ntype = -1;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[NODE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else *Ntype = Node[index].type;

    return(errcode);
}

int DLLEXPORT swmm_getLinkType(int index, int *Ltype)
///
/// Input:   index = Index of desired ID
///          Ltype = Link type (Based on enum SM_LinkType)
/// Return:  API Error
/// Purpose: Gets Link Type
{
    int errcode = 0;
    *Ltype = -1;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else *Ltype = Link[index].type;

    return(errcode);
}

int DLLEXPORT swmm_getLinkConnections(int index, int *Node1, int *Node2)
///
/// Input:   index = Index of desired ID
/// Output:  Node1 and Node2 indeces
/// Return:  API Error
/// Purpose: Gets link Connection ID Indeces
{
    int errcode = 0;
    *Node1 = -1;
    *Node2 = -1;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        *Node1 = Link[index].node1;
        *Node2 = Link[index].node2;
    }
    return(errcode);
}

int DLLEXPORT swmm_getLinkDirection(int index, signed char *value)
///
/// Input:   index = Index of desired ID
/// Output:  Link Direction (Only changes is slope < 0)
/// Return:  API Error
/// Purpose: Gets Link Direction
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        *value = Link[index].direction;
    }
    return(errcode);
}

int DLLEXPORT swmm_getNodeParam(int index, int Param, double *value)
///
/// Input:   index = Index of desired ID
///          param = Parameter desired (Based on enum SM_NodeProperty)
/// Output:  value = value to be output
/// Return:  API Error
/// Purpose: Gets Node Parameter
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[NODE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch(Param)
        {
            case SM_INVERTEL:
                *value = Node[index].invertElev * UCF(LENGTH); break;
            case SM_FULLDEPTH:
                *value = Node[index].fullDepth * UCF(LENGTH); break;
            case SM_SURCHDEPTH:
                *value = Node[index].surDepth * UCF(LENGTH); break;
            case SM_PONDAREA:
                *value = Node[index].pondedArea * UCF(LENGTH) * UCF(LENGTH); break;
            case SM_INITDEPTH:
                *value = Node[index].initDepth * UCF(LENGTH); break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_setNodeParam(int index, int Param, double value)
///
/// Input:   index = Index of desired ID
///          param = Parameter desired (Based on enum SM_NodeProperty)
///          value = value to be input
/// Return:  API Error
/// Purpose: Sets Node Parameter
{
    int errcode = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
     // Check if Simulation is Running
    else if(swmm_IsStartedFlag() == TRUE)
    {
        errcode = ERR_API_SIM_NRUNNING;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[NODE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch(Param)
        {
            case SM_INVERTEL:
                Node[index].invertElev = value / UCF(LENGTH); break;
            case SM_FULLDEPTH:
                Node[index].fullDepth = value / UCF(LENGTH); break;
            case SM_SURCHDEPTH:
                Node[index].surDepth = value / UCF(LENGTH); break;
            case SM_PONDAREA:
                Node[index].pondedArea = value / ( UCF(LENGTH) * UCF(LENGTH) ); break;
            case SM_INITDEPTH:
                Node[index].initDepth = value / UCF(LENGTH); break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    // Re-validated a node BEM 1/20/2017 Probably need to re-validate connecting links
    //node_validate(index)
    return(errcode);
}

int DLLEXPORT swmm_getLinkParam(int index, int Param, double *value)
///
/// Input:   index = Index of desired ID
///          param = Parameter desired (Based on enum SM_LinkProperty)
/// Output:  value = value to be output
/// Return:  API Error
/// Purpose: Gets Link Parameter
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch(Param)
        {
            case SM_OFFSET1:
                *value = Link[index].offset1 * UCF(LENGTH); break;
            case SM_OFFSET2:
                *value = Link[index].offset2 * UCF(LENGTH); break;
            case SM_INITFLOW:
                *value = Link[index].q0 * UCF(FLOW); break;
            case SM_FLOWLIMIT:
                *value = Link[index].qLimit * UCF(FLOW); break;
            case SM_INLETLOSS:
                *value = Link[index].cLossInlet; break;
            case SM_OUTLETLOSS:
                *value = Link[index].cLossOutlet; break;
            case SM_AVELOSS:
                *value = Link[index].cLossAvg; break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_setLinkParam(int index, int Param, double value)
///
/// Input:   index = Index of desired ID
///          param = Parameter desired (Based on enum SM_LinkProperty)
///          value = value to be output
/// Return:  API Error
/// Purpose: Sets Link Parameter
{
    int errcode = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch(Param)
        {
            // offset1
            case SM_OFFSET1:
                // Check if Simulation is Running
                if(swmm_IsStartedFlag() == TRUE)
                {
                    errcode = ERR_API_SIM_NRUNNING; break;
                }
                Link[index].offset1 = value / UCF(LENGTH); break;
            case SM_OFFSET2:
                // Check if Simulation is Running
                if(swmm_IsStartedFlag() == TRUE)
                {
                    errcode = ERR_API_SIM_NRUNNING; break;
                }
                Link[index].offset2 = value / UCF(LENGTH); break;
            case SM_INITFLOW:
                Link[index].q0 = value / UCF(FLOW); break;
            case SM_FLOWLIMIT:
                Link[index].qLimit = value / UCF(FLOW); break;
            case SM_INLETLOSS:
                Link[index].cLossInlet = value; break;
            case SM_OUTLETLOSS:
                Link[index].cLossOutlet = value; break;
            case SM_AVELOSS:
                Link[index].cLossAvg = value; break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
        // re-validated link
        //link_validate(index);
    }

    return(errcode);
}


int DLLEXPORT swmm_getSubcatchParam(int index, int Param, double *value)
///
/// Input:   index = Index of desired ID
///          param = Parameter desired (Based on enum SM_SubcProperty)
/// Output:  value = value to be output
/// Return:  API Error
/// Purpose: Gets Subcatchment Parameter
{
    int errcode = 0;
    *value = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch(Param)
        {
            case SM_WIDTH:
                *value = Subcatch[index].width * UCF(LENGTH); break;
            case SM_AREA:
                *value = Subcatch[index].area * UCF(LANDAREA); break;
            case SM_FRACIMPERV:
                *value = Subcatch[index].fracImperv; break;
            case SM_SLOPE:
                *value = Subcatch[index].slope; break;
            case SM_CURBLEN:
                *value = Subcatch[index].curbLength * UCF(LENGTH); break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_setSubcatchParam(int index, int Param, double value)
///
/// Input:   index = Index of desired ID
///          param = Parameter desired (Based on enum SM_SubcProperty)
///          value = value to be output
/// Return:  API Error
/// Purpose: Sets Subcatchment Parameter
{
    int errcode = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
     // Check if Simulation is Running
    else if(swmm_IsStartedFlag() == TRUE)
    {
        errcode = ERR_API_SIM_NRUNNING;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch(Param)
        {
            case SM_WIDTH:
                Subcatch[index].width = value / UCF(LENGTH); break;
            case SM_AREA:
                Subcatch[index].area = value / UCF(LANDAREA); break;
            case SM_FRACIMPERV:
                break; //Subcatch[index].fracImperv; break;
                // Cannot Open Function just yet.  Need
                // to adjust some internal functions to
                // ensure parameters are recalculated
                // = MIN(value, 100.0) / 100.0; break;
            case SM_SLOPE:
                Subcatch[index].slope = value; break;
            case SM_CURBLEN:
                Subcatch[index].curbLength = value / UCF(LENGTH); break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
        //re-validate subcatchment
        subcatch_validate(index); // incorprate callback here
    }

    return(errcode);
}

int DLLEXPORT swmm_getSubcatchOutConnection(int index, int *type, int *out_index)
///
/// Input:   index = Index of desired ID
///         (Subcatchments can load to Node or another Subcatchment)
/// Output:  Type of Object
///          Index of Object
/// Return:  API Error
/// Purpose: Gets Subcatchment Connection ID Indeces for either Node or Subcatchment
{
    int errcode = 0;
    *type = -1;
    *out_index = -1;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        if (Subcatch[index].outNode == -1 && Subcatch[index].outSubcatch == -1)
        {
            *out_index = index; // Case of self Loading subcatchment
            *type = SUBCATCH;
        }
        if (Subcatch[index].outNode >= 0)
        {
            *out_index = Subcatch[index].outNode;
            *type = NODE;
        }
        if (Subcatch[index].outSubcatch >= 0)
        {
            *out_index = Subcatch[index].outSubcatch;
            *type = SUBCATCH;
        }
    }
    return(errcode);
}


int DLLEXPORT swmm_getLidUCount(int index, int *value)
// Input:   index = Index of desired subcatchment 
// Output:  int = number of lid units for subcatchment 
// Return:  number of lid units for subcatchment
// Purpose: count number of lid units for subcatchment
{
    int errcode = 0;
    
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        *value = lid_getLidUnitCount(index);
    }
    
    return (errcode);
}


int DLLEXPORT swmm_getLidUParam(int index, int lidIndex, int param, double *value)
//
// Input:   index = Index of desired subcatchment 
//          lidIndex = Index of desired lid unit (subcatchment allow for multiple lid units)
//          param = Parameter desired (Based on enum SM_LidUProperty)
// Output:  value = value to be output
// Return:  API Error
// Purpose: Gets Lid Unit Parameter
{
    int errcode = 0;
    TLidUnit* lidUnit;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidUnit = lid_getLidUnit(index, lidIndex, &errcode);

        // There are no Lid Units defined for the subcatchments
        if(lidUnit)
        {
            switch(param)
            {
                case SM_UNITAREA:
                    *value = lidUnit->area * SQR(UCF(LENGTH)); break;
                case SM_FWIDTH:
                    *value = lidUnit->fullWidth * UCF(LENGTH); break;
                case SM_BWIDTH:
                    *value = lidUnit->botWidth * UCF(LENGTH); break;
                case SM_INITSAT:
                    *value = lidUnit->initSat * 100; break;
                case SM_FROMIMPERV:
                    *value = lidUnit->fromImperv * 100; break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
        }
    }

    return(errcode);
}


int DLLEXPORT swmm_setLidUParam(int index, int lidIndex, int Param, double value)
//
// Input:   index = Index of desired subcatchment 
//          lidIndex = Index of desired lid unit (subcatchment allow for multiple lid units)
//          param = Parameter desired (Based on enum SM_LidUProperty)
//          value = replacement
// Return:  API Error
// Purpose: Gets Lid Unit Parameter
{
    int errcode = 0;
    TLidUnit* lidUnit;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidUnit = lid_getLidUnit(index, lidIndex, &errcode);

        // There are no Lid Units defined for the subcatchments
        if(lidUnit)
        {
            switch(Param)
            {
                case SM_UNITAREA:
                    lidUnit->area = value / SQR(UCF(LENGTH)); break;
                case SM_FWIDTH:
                    lidUnit->fullWidth = value / UCF(LENGTH); break;
                case SM_BWIDTH:
                    lidUnit->botWidth = value / UCF(LENGTH); break;
                case SM_INITSAT:
                    lidUnit->initSat = value / 100; break;
                case SM_FROMIMPERV:
                    lidUnit->fromImperv = value / 100; break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
            // Validate Lid Unit 
            // Only see function for validating all the lid units within subcatchments (lidGroup)
            lid_validateLidGroup(index);
        }
    }

    return(errcode);
}


int DLLEXPORT swmm_getLidUOption(int index, int lidIndex, int param, int *value)
//
// Input:   index = Index of desired subcatchment 
//          lidIndex = Index of desired lid unit (subcatchment allow for multiple lid units)
//          param = Parameter desired (Based on enum SM_LidUOption)
// Output:  value = value to be output
// Return:  API Error
// Purpose: Gets Lid Unit Option Parameter
{
    int errcode = 0;
    TLidUnit* lidUnit;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidUnit = lid_getLidUnit(index, lidIndex, &errcode);

        // There are no Lid Units defined for the subcatchments
        if(lidUnit)
        {
            switch(param)
            {
                case SM_INDEX:
                    *value = lidUnit->lidIndex; break;
                case SM_NUMBER:
                    *value = lidUnit->number; break;
                case SM_TOPERV:
                    *value = lidUnit->toPerv; break;
                case SM_DRAINSUB:
                    *value = lidUnit->drainSubcatch; break;
                case SM_DRAINNODE:
                    *value = lidUnit->drainNode; break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
        }
    }

    return(errcode);
}


int DLLEXPORT swmm_setLidUOption(int index, int lidIndex, int param, int value)
//
// Input:   index = Index of desired subcatchment 
//          lidIndex = Index of desired lid unit (subcatchment allow for multiple lid units)
//          param = Parameter desired (Based on enum SM_LidUOption)
//          value = replacement value
// Return:  API Error
// Purpose: Sets Lid Unit Option
{
    int errcode = 0;
    TLidUnit* lidUnit;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidUnit = lid_getLidUnit(index, lidIndex, &errcode);

        // There are no Lid Units defined for the subcatchments
        if(lidUnit)
        {
            switch(param)
            {
                case SM_INDEX:
                    lidUnit->lidIndex = value; break;
                case SM_NUMBER:
                    lidUnit->number = value; break;
                case SM_TOPERV:
                    lidUnit->toPerv = value; break;
                case SM_DRAINSUB:
                    lidUnit->drainSubcatch = value; break;
                case SM_DRAINNODE:
                    lidUnit->drainNode = value; break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
            // Validate Lid Unit 
            // Only see function for validating all the lid units within subcatchments (lidGroup)
            lid_validateLidGroup(index);
        }
    }

    return(errcode);
}


int DLLEXPORT swmm_getLidCOverflow(int lidControlIndex, char *condition)
//
// Input:   lidControlIndex = Index of desired lid control
// Output:  condition = value to be output
// Return:  API Error
// Purpose: Get Lid Control Surface Immediate Overflow Condition
{
    int errcode = 0;
    TLidProc*  lidProc;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(lidControlIndex < 0 || lidControlIndex >= Nobjects[LID])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidProc = lid_getLidProc(lidControlIndex);
        *condition = lidProc->surface.canOverflow;
    }
    return(errcode);
}


int DLLEXPORT swmm_setLidCOverflow(int lidControlIndex, char condition)
//
// Input:   lidControlIndex = Index of desired lid control
//          condition = replacement value
// Return:  API Error
// Purpose: Sets Lid Control Surface Immediate Overflow Condition
{
    int errcode = 0;
    TLidProc*  lidProc;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(lidControlIndex < 0 || lidControlIndex >= Nobjects[LID])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidProc = lid_getLidProc(lidControlIndex);
        if (lidProc != NULL)
        {
            lidProc->surface.canOverflow = condition;
            //re-validate lid control
            lid_validateLidProc(lidControlIndex);
        }
    }

    return(errcode);
}


int DLLEXPORT swmm_getLidCParam(int lidControlIndex, int layerIndex, int param, double *value)
//
// Input:   lidControlIndex = Index of desired lid control
//          layerIndex = Index of desired lid control layer (Based on enum SM_LidLayers)
//          param = Parameter desired (Based on enum SM_LidLayersProperty)
// Output:  value = value to be output
// Return:  API Error
// Purpose: Gets Lid Control Layer Parameter
{
    int errcode = 0;
    TLidProc*  lidProc;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(lidControlIndex < 0 || lidControlIndex >= Nobjects[LID])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidProc = lid_getLidProc(lidControlIndex);

        switch(layerIndex)
        {
        case SM_SURFACE:
            switch(param)
            {
            case SM_THICKNESS:
                *value = lidProc->surface.thickness * UCF(RAINDEPTH); break;
            case SM_VOIDFRAC:
                *value = 1 - lidProc->surface.voidFrac; break;
            case SM_ROUGHNESS:
                *value = lidProc->surface.roughness; break;
            case SM_SURFSLOPE:
                *value = lidProc->surface.surfSlope * 100; break;
            case SM_SIDESLOPE:
                *value = lidProc->surface.sideSlope; break;
            case SM_ALPHA:
                *value = lidProc->surface.alpha; break;
            default:
                errcode = ERR_API_OUTBOUNDS; break;
            }
            break;
        case SM_SOIL:
            switch(param)
            {
            case SM_THICKNESS:
                *value = lidProc->soil.thickness * UCF(RAINDEPTH); break;
            case SM_POROSITY:
                *value = lidProc->soil.porosity; break;
            case SM_FIELDCAP:
                *value = lidProc->soil.fieldCap; break;
            case SM_WILTPOINT:
                *value = lidProc->soil.wiltPoint; break;
            case SM_KSAT:
                *value = lidProc->soil.kSat * UCF(RAINFALL); break;
            case SM_KSLOPE:
                *value = lidProc->soil.kSlope; break;
            case SM_SUCTION:
                *value = lidProc->soil.suction * UCF(RAINDEPTH); break;
            default:
                errcode = ERR_API_OUTBOUNDS; break;
            }
            break;
        case SM_STOR:
            switch(param)
            {
            case SM_THICKNESS:
                *value = lidProc->storage.thickness * UCF(RAINDEPTH); break;
            case SM_VOIDFRAC:
                if (lidProc->storage.voidFrac < 1)
                {
                    *value = lidProc->storage.voidFrac / (1 - lidProc->storage.voidFrac);
                }
                else
                {
                    *value = lidProc->storage.voidFrac;
                }
                break;
            case SM_KSAT:
                *value = lidProc->storage.kSat * UCF(RAINFALL); break;
            case SM_CLOGFACTOR:
                if (lidProc->storage.thickness > 0.0)
                {
                    *value = lidProc->storage.clogFactor / 
                            (lidProc->storage.thickness * 
                            lidProc->storage.voidFrac); break;
                }
                else
                {
                    *value = lidProc->pavement.clogFactor;
                }
            default:
                errcode = ERR_API_OUTBOUNDS; break;
            }
            break;
        case SM_PAVE:
            switch(param)
            {
            case SM_THICKNESS:
                *value = lidProc->pavement.thickness * UCF(RAINDEPTH); break;
            case SM_VOIDFRAC:
                if (lidProc->pavement.voidFrac < 1)
                {
                    *value = lidProc->pavement.voidFrac / (1 - lidProc->pavement.voidFrac);
                }
                else
                {
                    *value = lidProc->pavement.voidFrac;
                }
                break;
            case SM_IMPERVFRAC:
                *value = lidProc->pavement.impervFrac; break;
            case SM_KSAT:
                *value = lidProc->pavement.kSat * UCF(RAINFALL); break;
            case SM_CLOGFACTOR:
                if (lidProc->pavement.thickness > 0.0)
                {
                    *value = lidProc->pavement.clogFactor / 
                            (lidProc->pavement.thickness * 
                            lidProc->pavement.voidFrac * 
                            (1 - lidProc->pavement.impervFrac)); break;
                }
                else
                {
                    *value = lidProc->pavement.clogFactor;
                }
            case SM_REGENDAYS:
                *value = lidProc->pavement.regenDays; break;
            case SM_REGENDEGREE:
                *value = lidProc->pavement.regenDegree; break;
            default:
                errcode = ERR_API_OUTBOUNDS; break;
            }
            break;
        case SM_DRAIN:
            switch(param)
            {
            case SM_COEFF:
                *value = lidProc->drain.coeff; break;
            case SM_EXPON:
                *value = lidProc->drain.expon; break;
            case SM_OFFSET:
                *value = lidProc->drain.offset * UCF(RAINDEPTH); break;
            case SM_DELAY:
                *value = lidProc->drain.delay / 3600; break;
            case SM_HOPEN:
                *value = lidProc->drain.hOpen * UCF(RAINDEPTH); break;
            case SM_HCLOSE:
                *value = lidProc->drain.hClose * UCF(RAINDEPTH); break;
            default:
                errcode = ERR_API_OUTBOUNDS; break;
            }
            break;
        case SM_DRAINMAT:
            switch(param)
            {
            case SM_THICKNESS:
                *value = lidProc->drainMat.thickness * UCF(RAINDEPTH); break;
            case SM_VOIDFRAC:
                *value = lidProc->drainMat.voidFrac; break;
            case SM_ROUGHNESS:
                *value = lidProc->drainMat.roughness; break;
            case SM_ALPHA:
                *value = lidProc->drainMat.alpha; break;
            default:
                errcode = ERR_API_OUTBOUNDS; break;
            }
            break;
        default:
            errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}


int DLLEXPORT swmm_setLidCParam(int lidControlIndex, int layerIndex, int param, double value)
//
// Input:   lidControlIndex = Index of desired lid control
//          layerIndex = Index of desired lid control layer (Based on enum SM_LidLayers)
//          param = Parameter desired (Based on enum SM_LidLayersProperty)
//          value = replacement value
// Return:  API Error
// Purpose: Sets Lid Control Layer Parameter
{
    int errcode = 0;
    TLidProc*  lidProc;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if subcatchment index is within bounds
    else if(lidControlIndex < 0 || lidControlIndex >= Nobjects[LID])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidProc = lid_getLidProc(lidControlIndex);

        switch(layerIndex)
        {
            case SM_SURFACE:
                switch(param)
                {
                    case SM_THICKNESS:
                        lidProc->surface.thickness = value / UCF(RAINDEPTH); break;
                    case SM_VOIDFRAC:
                        lidProc->surface.voidFrac = 1 - value; break;
                    case SM_ROUGHNESS:
                        lidProc->surface.roughness = value; break;
                    case SM_SURFSLOPE:
                        lidProc->surface.surfSlope = value / 100; break;
                    case SM_SIDESLOPE:
                        lidProc->surface.sideSlope = value; break;
                    //case SM_ALPHA:
                    //    lidProc->surface.alpha = value; break;
                    default:
                        errcode = ERR_API_OUTBOUNDS; break;
                }
                break;
            case SM_SOIL:
                switch(param)
                {
                    case SM_THICKNESS:
                        lidProc->soil.thickness = value / UCF(RAINDEPTH); break;
                    case SM_POROSITY:
                        lidProc->soil.porosity = value; break;
                    case SM_FIELDCAP:
                        lidProc->soil.fieldCap = value; break;
                    case SM_WILTPOINT:
                        lidProc->soil.wiltPoint = value; break;
                    case SM_KSAT:
                        lidProc->soil.kSat = value / UCF(RAINFALL); break;
                    case SM_KSLOPE:
                        lidProc->soil.kSlope = value; break;
                    case SM_SUCTION:
                        lidProc->soil.suction = value / UCF(RAINDEPTH); break;
                    default:
                        errcode = ERR_API_OUTBOUNDS; break;
                }
                break;
            case SM_STOR:
                switch(param)
                {
                    case SM_THICKNESS:
                        if (lidProc->storage.thickness > 0)
                        {
                            lidProc->storage.clogFactor /= lidProc->storage.thickness;
                            lidProc->storage.thickness = value / UCF(RAINDEPTH);
                            lidProc->storage.clogFactor *= lidProc->storage.thickness;
                        }
                        else
                        {
                            lidProc->storage.thickness = value / UCF(RAINDEPTH);
                        }
                        break;
                    case SM_VOIDFRAC:
                        if (lidProc->storage.voidFrac > 0)
                        {
                            lidProc->storage.clogFactor /= lidProc->storage.voidFrac;
                            lidProc->storage.voidFrac = value / (value + 1); 
                            lidProc->storage.clogFactor *= lidProc->storage.voidFrac;
                        }
                        else
                        {
                            lidProc->storage.voidFrac = value / (value + 1);
                        }

                        break;
                    case SM_KSAT:
                        lidProc->storage.kSat = value / UCF(RAINFALL); break;
                    case SM_CLOGFACTOR:
                        lidProc->storage.clogFactor = value * 
                        lidProc->storage.thickness * 
                        lidProc->storage.voidFrac; 
                        break;
                    default:
                        errcode = ERR_API_OUTBOUNDS; break;
                }
                break;
            case SM_PAVE:
                switch(param)
                {
                    case SM_THICKNESS:
                        if (lidProc->pavement.thickness > 0)
                        {
                            lidProc->pavement.clogFactor /= lidProc->pavement.thickness;
                            lidProc->pavement.thickness = value / UCF(RAINDEPTH);
                            lidProc->pavement.clogFactor *= lidProc->pavement.thickness;
                        }
                        else
                        {
                            lidProc->pavement.thickness = value / UCF(RAINDEPTH);
                        }
                        break;
                    case SM_VOIDFRAC:
                        if (lidProc->pavement.voidFrac > 0)
                        {
                            lidProc->pavement.clogFactor /= lidProc->pavement.voidFrac;
                            lidProc->pavement.voidFrac = value / (value + 1);
                            lidProc->pavement.clogFactor *= lidProc->pavement.voidFrac;
                        }
                        else
                        {
                            lidProc->pavement.voidFrac = value / (value + 1);
                        }
                        break;
                    case SM_IMPERVFRAC:
                        if (lidProc->pavement.impervFrac > 0)
                        {
                            lidProc->pavement.clogFactor /= (1 - lidProc->pavement.impervFrac);
                            lidProc->pavement.impervFrac = value;
                            lidProc->pavement.clogFactor *= (1 - lidProc->pavement.impervFrac);
                        }
                        else
                        {
                            lidProc->pavement.impervFrac = value;
                        }
                        break;
                    case SM_KSAT:
                        lidProc->pavement.kSat = value / UCF(RAINFALL); break;
                    case SM_CLOGFACTOR:
                        lidProc->pavement.clogFactor = value *
                        lidProc->pavement.thickness * 
                        lidProc->pavement.voidFrac *
                        (1.0 - lidProc->pavement.impervFrac); 
                        break;
                    case SM_REGENDAYS:
                        lidProc->pavement.regenDays = value; break;
                    case SM_REGENDEGREE:
                        lidProc->pavement.regenDegree = value; break;
                    default:
                        errcode = ERR_API_OUTBOUNDS; break;
                }
                break;
            case SM_DRAIN:
                switch(param)
                {
                    case SM_COEFF:
                        lidProc->drain.coeff = value; break;
                    case SM_EXPON:
                        lidProc->drain.expon = value; break;
                    case SM_OFFSET:
                        lidProc->drain.offset = value / UCF(RAINDEPTH); break;
                    case SM_DELAY:
                        lidProc->drain.delay = value * 3600; break;
                    case SM_HOPEN:
                        lidProc->drain.hOpen = value / UCF(RAINDEPTH); break;
                    case SM_HCLOSE:
                        lidProc->drain.hClose = value / UCF(RAINDEPTH); break;
                    default:
                        errcode = ERR_API_OUTBOUNDS; break;
                }
                break;
            case SM_DRAINMAT:
                switch(param)
                {
                    case SM_THICKNESS:
                        lidProc->drainMat.thickness = value / UCF(RAINDEPTH); break;
                    case SM_VOIDFRAC:
                        lidProc->drainMat.voidFrac = value; break;
                    case SM_ROUGHNESS:
                        lidProc->drainMat.roughness = value; break;
                    //case SM_ALPHA:
                    //    lidProc->drainMat.alpha = value; break;
                    default:
                        errcode = ERR_API_OUTBOUNDS; break;
                }
                break;
            default:
                errcode = ERR_API_OUTBOUNDS; break;
        }

    }
    if (errcode == ERR_NONE)
    {
        lid_validateLidProc(lidControlIndex);
    }
    return(errcode);
}

//-------------------------------
// Active Simulation Results API
//-------------------------------

int DLLEXPORT swmm_getCurrentDateTimeStr(char *dtimestr)
///
/// Output:  DateTime String
/// Return:  API Error
/// Purpose: Get the current simulation time
{
    //strcpy(dtimestr,"");
    //Provide Empty Character Array
    char     theDate[12];
    char     theTime[9];
    char     _DTimeStr[22];
    DateTime currentTime;

    // Check if Simulation is Running
    if(swmm_IsStartedFlag() == FALSE) return(ERR_API_SIM_NRUNNING);

    // Fetch Current Time
    currentTime = getDateTime(NewRoutingTime);

    // Convert To Char
    datetime_dateToStr(currentTime, theDate);
    datetime_timeToStr(currentTime, theTime);

    strcpy(_DTimeStr, theDate);
    strcat(_DTimeStr, " ");
    strcat(_DTimeStr, theTime);

    strcpy(dtimestr, _DTimeStr);
    return(0);
}


int DLLEXPORT swmm_getNodeResult(int index, int type, double *result)
///
/// Input:   index = Index of desired ID
///          type = Result Type (SM_NodeResult)
/// Output:  result = result data desired (byref)
/// Return:  API Error
/// Purpose: Gets Node Simulated Value at Current Time
{
    int errcode = 0;
    *result = 0;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[NODE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch (type)
        {
            case SM_TOTALINFLOW:
                *result = Node[index].inflow * UCF(FLOW); break;
            case SM_TOTALOUTFLOW:
                *result = Node[index].outflow * UCF(FLOW); break;
            case SM_LOSSES:
                *result = Node[index].losses * UCF(FLOW); break;
            case SM_NODEVOL:
                *result = Node[index].newVolume * UCF(VOLUME); break;
            case SM_NODEFLOOD:
                *result = Node[index].overflow * UCF(FLOW); break;
            case SM_NODEDEPTH:
                *result = Node[index].newDepth * UCF(LENGTH); break;
            case SM_NODEHEAD:
                *result = (Node[index].newDepth
                            + Node[index].invertElev) * UCF(LENGTH); break;
            case SM_LATINFLOW:
                *result = Node[index].newLatFlow * UCF(FLOW); break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_getLinkResult(int index, int type, double *result)
///
/// Input:   index = Index of desired ID
///          type = Result Type (SM_LinkResult)
/// Output:  result = result data desired (byref)
/// Return:  API Error
/// Purpose: Gets Link Simulated Value at Current Time
{
    int errcode = 0;
    *result = 0;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch (type)
        {
            case SM_LINKFLOW:
                *result = Link[index].newFlow * UCF(FLOW) ; break;
            case SM_LINKDEPTH:
                *result = Link[index].newDepth * UCF(LENGTH); break;
            case SM_LINKVOL:
                *result = Link[index].newVolume * UCF(VOLUME); break;
            case SM_USSURFAREA:
                *result = Link[index].surfArea1 * UCF(LENGTH) * UCF(LENGTH); break;
            case SM_DSSURFAREA:
                *result = Link[index].surfArea2 * UCF(LENGTH) * UCF(LENGTH); break;
            case SM_SETTING:
                *result = Link[index].setting; break;
            case SM_TARGETSETTING:
                *result = Link[index].targetSetting; break;
            case SM_FROUDE:
                *result = Link[index].froude; break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_getSubcatchResult(int index, int type, double *result)
///
/// Input:   index = Index of desired ID
///          type = Result Type (SM_SubcResult)
/// Output:  result = result data desired (byref)
/// Return:  API Error
/// Purpose: Gets Subcatchment Simulated Value at Current Time
{
    int errcode = 0;
    *result = 0;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        switch (type)
        {
            case SM_SUBCRAIN:
                *result = Subcatch[index].rainfall * UCF(RAINFALL); break;
            case SM_SUBCEVAP:
                *result = Subcatch[index].evapLoss * UCF(EVAPRATE); break;
            case SM_SUBCINFIL:
                *result = Subcatch[index].infilLoss * UCF(RAINFALL); break;
            case SM_SUBCRUNON:
                *result = Subcatch[index].runon * UCF(FLOW); break;
            case SM_SUBCRUNOFF:
                *result = Subcatch[index].newRunoff * UCF(FLOW); break;
            case SM_SUBCSNOW:
                *result = Subcatch[index].newSnowDepth * UCF(RAINDEPTH); break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_getSubcatchPollut(int index, int type, double **PollutArray)
///
/// Input:   index = Index of desired ID
///          type = Result Type (SM_SubcPollut)
/// Output:  PollutArray pointer (pollutant data desired, byref)
/// Return:  API Error
/// Purpose: Gets Subcatchment Simulated Pollutant Value at Current Time
{
    int p;
    int errcode = 0;
    double a;
    double* result;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else if (MEMCHECK(result = newDoubleArray(Nobjects[POLLUT])))
    {
        errcode = ERR_MEMORY;
    }

    else
    {
        switch (type)
        {
            case SM_BUILDUP:
            {
                a = Subcatch[index].area;
                for (p = 0; p < Nobjects[POLLUT]; p++)
                {
                    result[p] = Subcatch[index].surfaceBuildup[p] /
                        (a * UCF(LANDAREA));
                } *PollutArray = result;
            } break;
            case SM_CPONDED:
            {
                for (p = 0; p < Nobjects[POLLUT]; p++)
                {
                    result[p] = Subcatch[index].concPonded[p] / LperFT3;
                } *PollutArray = result;
            } break;
            default: errcode = ERR_API_OUTBOUNDS; break;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_getGagePrecip(int index, double **GageArray)
///
/// Input:   index = Index of desired ID
/// Output:  GageArray pointer (three elements)
/// Return:  API Error
/// Purpose: Gets the precipitation value in the gage.
{
    int errcode = 0;
    double rainfall = 0;
    double snowfall = 0;
    double total = 0;
    double* temp;

    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[GAGE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else if (MEMCHECK(temp = newDoubleArray(3)))
    {
        errcode = ERR_MEMORY;
    }
    // Read the rainfall value
    else
    {
        total = gage_getPrecip(index, &rainfall, &snowfall);
        temp[0] = total * UCF(RAINFALL);
        temp[1] = rainfall * UCF(RAINFALL);
        temp[2] = snowfall * UCF(RAINFALL);
        *GageArray = temp;
    }
    return(errcode);
}

int DLLEXPORT swmm_getNodeStats(int index, SM_NodeStats *nodeStats)
///
/// Output:  Node Stats Structure (SM_NodeStats)
/// Return:  API Error
/// Purpose: Gets Node Stats and Converts Units
{
    int errorcode = stats_getNodeStat(index, nodeStats);

    if (errorcode == 0)
    {
        // Current Average Depth
        nodeStats->avgDepth *= (UCF(LENGTH) / (double)StepCount);
        // Current Maximum Depth
        nodeStats->maxDepth *= UCF(LENGTH);
        // Current Maximum Lateral Inflow
        nodeStats->maxLatFlow *= UCF(FLOW);
        // Current Maximum Inflow
        nodeStats->maxInflow *= UCF(FLOW);
        // Cumulative Lateral Inflow
        nodeStats->totLatFlow *= UCF(VOLUME);
        // Time Courant Critical (hrs)
        nodeStats->timeCourantCritical /= 3600.0;
        // Cumulative Flooded Volume
        nodeStats->volFlooded *= UCF(VOLUME);
        // Time Flooded (hrs)
        nodeStats->timeFlooded /= 3600.0;
        // Current Maximum Overflow
        nodeStats->maxOverflow *= UCF(FLOW);
        // Current Maximum Ponding Volume
        nodeStats->maxPondedVol *= UCF(VOLUME);
        // Time Surcharged
        nodeStats->timeSurcharged /= 3600.0;
    }
    return (errorcode);
}

int DLLEXPORT swmm_getNodeTotalInflow(int index, double *value)
///
/// Input:   Node Index
/// Output:  Node Total inflow Volume.
/// Return:  API Error
/// Purpose: Get Node Total Inflow Volume.
{
    //*value = 0;
    int errorcode = massbal_getNodeTotalInflow(index, value);

    if (errorcode == 0)
    {
        *value *= UCF(VOLUME);
    }

    return(errorcode);
}

int DLLEXPORT swmm_getStorageStats(int index, SM_StorageStats *storageStats)
///
/// Output:  Storage Node Stats Structure (SM_StorageStats)
/// Return:  API Error
/// Purpose: Gets Storage Node Stats and Converts Units
{
    int errorcode = stats_getStorageStat(index, storageStats);

    if (errorcode == 0)
    {
        // Initial Volume
        storageStats->initVol *= UCF(VOLUME);
        // Current Average Volume
        storageStats->avgVol *= (UCF(VOLUME) / (double)StepCount);
        // Current Maximum Volume
        storageStats->maxVol *= UCF(VOLUME);
        // Current Maximum Flow
        storageStats->maxFlow *= UCF(FLOW);
        // Current Evaporation Volume
        storageStats->evapLosses *= UCF(VOLUME);
        // Current Exfiltration Volume
        storageStats->exfilLosses *= UCF(VOLUME);
    }

    return (errorcode);
}

int DLLEXPORT swmm_getOutfallStats(int index, SM_OutfallStats *outfallStats)
///
/// Output:  Outfall Stats Structure (SM_OutfallStats)
/// Return:  API Error
/// Purpose: Gets Outfall Node Stats and Converts Units
/// Note:    Caller is responsible for calling swmm_freeOutfallStats
///          to free the pollutants array.
{
    int p;
    int errorcode = stats_getOutfallStat(index, outfallStats);

    if (errorcode == 0)
    {
        // Current Average Flow
        if ( outfallStats->totalPeriods > 0 )
        {
            outfallStats->avgFlow *= (UCF(FLOW) / (double)outfallStats->totalPeriods);
        }
        else
        {
            outfallStats->avgFlow *= 0.0;
        }
        // Current Maximum Flow
        outfallStats->maxFlow *= UCF(FLOW);
        // Convert Mass Units
        if (Nobjects[POLLUT] > 0)
        {
            for (p = 0; p < Nobjects[POLLUT]; p++)
                outfallStats->totalLoad[p] *= (LperFT3 * Pollut[p].mcf);
                if (Pollut[p].units == COUNT)
                {
                    outfallStats->totalLoad[p] = LOG10(outfallStats->totalLoad[p]);
                }
        }
    }

    return (errorcode);
}

void DLLEXPORT swmm_freeOutfallStats(SM_OutfallStats *outfallStats)
///
/// Return:  API Error
/// Purpose: Frees Outfall Node Stats and Converts Units
/// Note:    API user is responsible for calling swmm_freeOutfallStats
///          since this function performs a memory allocation.
{
    FREE(outfallStats->totalLoad);
}



int DLLEXPORT swmm_getLinkStats(int index, SM_LinkStats *linkStats)
///
/// Output:  Link Stats Structure (SM_LinkStats)
/// Return:  API Error
/// Purpose: Gets Link Stats and Converts Units
{
    int errorcode = stats_getLinkStat(index, linkStats);

    if (errorcode == 0)
    {
        // Cumulative Maximum Flowrate
        linkStats->maxFlow *= UCF(FLOW);
        // Cumulative Maximum Velocity
        linkStats->maxVeloc *= UCF(LENGTH);
        // Cumulative Maximum Depth
        linkStats->maxDepth *= UCF(LENGTH);
        // Cumulative Time Normal Flow
        linkStats->timeNormalFlow /= 3600.0;
        // Cumulative Time Inlet Control
        linkStats->timeInletControl /= 3600.0;
        // Cumulative Time Surcharged
        linkStats->timeSurcharged /= 3600.0;
        // Cumulative Time Upstream Full
        linkStats->timeFullUpstream /= 3600.0;
        // Cumulative Time Downstream Full
        linkStats->timeFullDnstream /= 3600.0;
        // Cumulative Time Full Flow
        linkStats->timeFullFlow /= 3600.0;
        // Cumulative Time Capacity limited
        linkStats->timeCapacityLimited /= 3600.0;
        // Cumulative Time Courant Critical Flow
        linkStats->timeCourantCritical /= 3600.0;
    }

    return (errorcode);
}


int DLLEXPORT swmm_getPumpStats(int index, SM_PumpStats *pumpStats)
///
/// Output:  Pump Link Stats Structure (SM_PumpStats)
/// Return:  API Error
/// Purpose: Gets Pump Link Stats and Converts Units
{
    int errorcode = stats_getPumpStat(index, pumpStats);

    if (errorcode == 0)
    {
        // Cumulative Minimum Flow
        pumpStats->minFlow *= UCF(FLOW);
        // Cumulative Average Flow
        if (pumpStats->totalPeriods > 0)
        {
            pumpStats->avgFlow *= (UCF(FLOW) / (double)pumpStats->totalPeriods);
        }
        else
        {
            pumpStats->avgFlow *= 0.0;
        }
        // Cumulative Maximum Flow
        pumpStats->maxFlow *= UCF(FLOW);
        // Cumulative Pumping Volume
        pumpStats->volume *= UCF(VOLUME);
    }

    return (errorcode);
}


int DLLEXPORT swmm_getSubcatchStats(int index, SM_SubcatchStats **subcatchStats)
///
/// Output:  Subcatchment Stats Structure (SM_SubcatchStats)
/// Return:  API Error
/// Purpose: Gets Subcatchment Stats and Converts Units
{
  int errorcode = 0;
  double a;
  TSubcatchStats *tmp = (TSubcatchStats *)calloc(1, sizeof(TSubcatchStats));

  // Check if Open
  if (swmm_IsOpenFlag() == FALSE)
  {
    errorcode = ERR_API_INPUTNOTOPEN;
  }

  // Check if Simulation is Running
  else if (swmm_IsStartedFlag() == FALSE)
  {
    errorcode = ERR_API_SIM_NRUNNING;
  }

  // Check if object index is within bounds
  else if (index < 0 || index >= Nobjects[SUBCATCH])
  {
    errorcode = ERR_API_OBJECT_INDEX;
  }
    // Copy Structure
  else
  {
    memcpy(tmp, stats_getSubcatchStat(index), sizeof(TSubcatchStats));
    *subcatchStats = (SM_SubcatchStats *)tmp;

    a = Subcatch[index].area;

    // Cumulative Runon Volume
    (*subcatchStats)->runon *= UCF(VOLUME);
    // Cumulative Infiltration Volume
    (*subcatchStats)->infil *= UCF(VOLUME);
    // Cumulative Runoff Volume
    (*subcatchStats)->runoff *= UCF(VOLUME);
    // Maximum Runoff Rate
    (*subcatchStats)->maxFlow *= UCF(FLOW);
    // Cumulative Rainfall Depth
    (*subcatchStats)->precip *= (UCF(RAINDEPTH) / a);
    // Cumulative Evaporation Volume
    (*subcatchStats)->evap *= UCF(VOLUME);
  }

    return (errorcode);
}


int DLLEXPORT swmm_getSystemRoutingStats(SM_RoutingTotals *routingTot)
///
/// Output:  System Routing Totals Structure (SM_RoutingTotals)
/// Return:  API Error
/// Purpose: Gets System Flow Routing Totals and Converts Units
{
    int errorcode = massbal_getRoutingFlowTotal(routingTot);

    if (errorcode == 0)
    {
        // Cumulative Dry Weather Inflow Volume
        routingTot->dwInflow *= UCF(VOLUME);
        // Cumulative Wet Weather Inflow Volume
        routingTot->wwInflow *= UCF(VOLUME);
        // Cumulative Groundwater Inflow Volume
        routingTot->gwInflow *= UCF(VOLUME);
        // Cumulative I&I Inflow Volume
        routingTot->iiInflow *= UCF(VOLUME);
        // Cumulative External Inflow Volume
        routingTot->exInflow *= UCF(VOLUME);
        // Cumulative Flooding Volume
        routingTot->flooding *= UCF(VOLUME);
        // Cumulative Outflow Volume
        routingTot->outflow  *= UCF(VOLUME);
        // Cumulative Evaporation Loss
        routingTot->evapLoss *= UCF(VOLUME);
        // Cumulative Seepage Loss
        routingTot->seepLoss *= UCF(VOLUME);
        // Continuity Error
        routingTot->pctError *= 100;
    }

    return(errorcode);
}

int DLLEXPORT swmm_getSystemRunoffStats(SM_RunoffTotals *runoffTot)
///
/// Output:  System Runoff Totals Structure (SM_RunoffTotals)
/// Return:  API Error
/// Purpose: Gets System Runoff Totals and Converts Units
{
    int errorcode =  massbal_getRunoffTotal(runoffTot);

    if (errorcode == 0)
    {
        double TotalArea = massbal_getTotalArea();
        // Cumulative Rainfall Depth
        runoffTot->rainfall *= (UCF(RAINDEPTH) / TotalArea);
        // Cumulative Evaporation Volume
        runoffTot->evap *= UCF(VOLUME);
        // Cumulative Infiltration Volume
        runoffTot->infil *= UCF(VOLUME);
        // Cumulative Runoff Volume
        runoffTot->runoff *= UCF(VOLUME);
        // Cumulative Runon Volume
        runoffTot->runon *= UCF(VOLUME);
        // Cumulative Drain Volume
        runoffTot->drains *= UCF(VOLUME);
        // Cumulative Snow Removed Volume
        runoffTot->snowRemoved *= (UCF(RAINDEPTH) / TotalArea);
        // Initial Storage Volume
        runoffTot->initStorage *= (UCF(RAINDEPTH) / TotalArea);
        // Final Storage Volume
        runoffTot->finalStorage *= (UCF(RAINDEPTH) / TotalArea);
        // Initial Snow Cover Volume
        runoffTot->initSnowCover *= (UCF(RAINDEPTH) / TotalArea);
        // Final Snow Cover Volume
        runoffTot->finalSnowCover *= (UCF(RAINDEPTH) / TotalArea);
        // Continuity Error
        runoffTot->pctError *= 100;
    }

    return(errorcode);
}

int DLLEXPORT swmm_getLidUFluxRates(int index, int lidIndex, int layerIndex, double *result)
//
// Input:   index = Index of desired subcatchment 
//          lidIndex = Index of desired lid control (subcatchment allow for multiple lids)
//          layerIndex = Index of desired lid layer (default is surface)
// Output:  result = result data desired (byref)
// Return:  API Error
// Purpose: Gets Lid Unit Water Balance Simulated Value at Current Time
{
    int errcode = 0;
    TLidUnit* lidUnit;

    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidUnit = lid_getLidUnit(index, lidIndex, &errcode);

        // There are no Lid Units defined for the subcatchments
        if (lidUnit)
        {
            switch (layerIndex)
            {
                case SM_SURFACE:
                    *result = lidUnit->oldFluxRates[SM_SURFACE] * UCF(LENGTH); break;
                case SM_SOIL:
                    *result = lidUnit->oldFluxRates[SM_SOIL] * UCF(LENGTH); break;
                case SM_STORAGE:
                    *result = lidUnit->oldFluxRates[SM_STORAGE] * UCF(LENGTH); break;
                case SM_PAVE:
                    *result = lidUnit->oldFluxRates[SM_PAVE] * UCF(LENGTH); break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_getLidGResult(int index, int type, double *result)
//
// Input:   index = index of desired subcatchment 
//          type = type of result data desired
// Output:  result = result data desired 
// Return:  API Error
// Purpose: Gets Lid Group Data at Current Time
{
    int errcode = 0;
    TLidGroup lidGroup;

    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidGroup = lid_getLidGroup(index);

        if (!lidGroup)
        {
            errcode = ERR_API_UNDEFINED_LID;
        }
        else
        {
            switch (type)
            {   
                case SM_PERVAREA:
                    *result = lidGroup->pervArea * SQR(UCF(LENGTH)); break;
                case SM_FLOWTOPERV:
                    *result = lidGroup->flowToPerv * UCF(FLOW); break;
                case SM_OLDDRAINFLOW:
                    *result = lidGroup->oldDrainFlow * UCF(FLOW); break;
                case SM_NEWDRAINFLOW:
                    *result = lidGroup->newDrainFlow * UCF(FLOW); break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_getLidUResult(int index, int lidIndex, int type, double *result)
//
// Input:   index = Index of desired subcatchment 
//          lidIndex = Index of desired lid control (subcatchment allow for multiple lids)
// Output:  result = result data desired (byref)
// Return:  API Error
// Purpose: Gets Lid Unit Water Balance Simulated Value at Current Time
{
    int errcode = 0;
    TLidUnit* lidUnit;
    double    Tstep = 0;

    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[SUBCATCH])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        lidUnit = lid_getLidUnit(index, lidIndex, &errcode);

        // There are no Lid Units defined for the subcatchments
        if (lidUnit)
        {
            switch (type)
            {
                case SM_INFLOW:
                    *result = lidUnit->waterBalance.inflow * UCF(RAINDEPTH); break;
                case SM_EVAP:
                    *result = lidUnit->waterBalance.evap * UCF(RAINDEPTH); break;
                case SM_INFIL:
                    *result = lidUnit->waterBalance.infil * UCF(RAINDEPTH); break;
                case SM_SURFFLOW:
                    *result = lidUnit->waterBalance.surfFlow * UCF(RAINDEPTH); break;
                case SM_DRAINFLOW:
                    *result = lidUnit->waterBalance.drainFlow * UCF(RAINDEPTH); break;
                case SM_INITVOL:
                    *result = lidUnit->waterBalance.initVol * UCF(RAINDEPTH); break;
                case SM_FINALVOL:
                    *result = lidUnit->waterBalance.finalVol * UCF(RAINDEPTH); break;
                case SM_SURFDEPTH:
                    *result = lidUnit->surfaceDepth * UCF(RAINDEPTH); break;
                case SM_PAVEDEPTH:
                    *result = lidUnit->paveDepth * UCF(RAINDEPTH); break;
                case SM_SOILMOIST:
                    *result = lidUnit->soilMoisture; break;
                case SM_STORDEPTH:
                    *result = lidUnit->storageDepth * UCF(RAINDEPTH); break;
                case SM_DRYTIME:
                    *result = lidUnit->dryTime; break;
                case SM_OLDDRAINFLOW:
                    *result = lidUnit->oldDrainFlow * UCF(FLOW); break;
                case SM_NEWDRAINFLOW:
                    *result = lidUnit->newDrainFlow * UCF(FLOW); break;
                case SM_EVAPRATE:
                    *result = lidUnit->waterRate.evap * UCF(RAINFALL); break;
                case SM_NATIVEINFIL:
                    *result = lidUnit->waterRate.maxNativeInfil * UCF(RAINFALL); break;
                case SM_SURFINFLOW:
                    *result = lidUnit->waterRate.surfaceInflow * UCF(RAINFALL); break;
                case SM_SURFINFIL:
                    *result = lidUnit->waterRate.surfaceInfil * UCF(RAINFALL); break;
                case SM_SURFEVAP:
                    *result = lidUnit->waterRate.surfaceEvap * UCF(RAINFALL); break;
                case SM_SURFOUTFLOW:
                    *result = lidUnit->waterRate.surfaceOutflow * UCF(RAINFALL); break;
                case SM_PAVEEVAP:
                    *result = lidUnit->waterRate.paveEvap * UCF(RAINFALL); break;
                case SM_PAVEPERC:
                    *result = lidUnit->waterRate.pavePerc * UCF(RAINFALL); break;
                case SM_SOILEVAP:
                    *result = lidUnit->waterRate.soilEvap * UCF(RAINFALL); break;
                case SM_SOILPERC:
                    *result = lidUnit->waterRate.soilPerc * UCF(RAINFALL); break;
                case SM_STORAGEINFLOW:
                    *result = lidUnit->waterRate.storageInflow * UCF(RAINFALL); break;
                case SM_STORAGEEXFIL:
                    *result = lidUnit->waterRate.storageExfil * UCF(RAINFALL); break;
                case SM_STORAGEEVAP:
                    *result = lidUnit->waterRate.storageEvap * UCF(RAINFALL); break;
                case SM_STORAGEDRAIN:
                    *result = lidUnit->waterRate.storageDrain * UCF(RAINFALL); break;
                default:
                    errcode = ERR_API_OUTBOUNDS; break;
            }
        }
    }
    return(errcode);
}

//-------------------------------
// Setters API
//-------------------------------

int DLLEXPORT swmm_setLinkSetting(int index, double setting)
///
/// Input:   index = Index of desired ID
///          value = New Target Setting
/// Output:  returns API Error
/// Purpose: Sets Link open fraction (Weir, Orifice, Pump, and Outlet)
{
    DateTime currentTime;
    int errcode = 0;
    char _rule_[11] = "ToolkitAPI";

    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[LINK])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        // --- check that new setting lies within feasible limits
        if (setting < 0.0) setting = 0.0;
        if (Link[index].type != PUMP && setting > 1.0) setting = 1.0;

        Link[index].targetSetting = setting;

        // Use internal function to apply the new setting
        link_setSetting(index, 0.0);

        // Add control action to RPT file if desired flagged
        if (RptFlags.controls)
        {
            currentTime = getDateTime(NewRoutingTime);
            report_writeControlAction(currentTime, Link[index].ID, setting, _rule_);
        }
    }
    return(errcode);
}


int DLLEXPORT swmm_setNodeInflow(int index, double flowrate)
///
/// Input:   index = Index of desired ID
///          value = New Inflow Rate
/// Output:  returns API Error
/// Purpose: Sets new node inflow rate and holds until set again
{
    int errcode = 0;

    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[NODE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else
    {
        // Check to see if node has an assigned inflow object
        TExtInflow* inflow;

        // --- check if an external inflow object for this constituent already exists
        inflow = Node[index].extInflow;
        while (inflow)
        {
            if (inflow->param == -1) break;
            inflow = inflow->next;
        }

        if (!inflow)
        {
            int param = -1;        // FLOW (-1) or Pollutant Index
            int type = FLOW_INFLOW;// Type of inflow (FLOW)
            int tSeries = -1;      // No Time Series
            int basePat = -1;      // No Base Pattern
            double cf = 1.0;       // Unit Convert (Converted during validation)
            double sf = 1.0;       // Scaling Factor
            double baseline = 0.0; // Baseline Inflow Rate

            // Initializes Inflow Object
            errcode = inflow_setExtInflow(index, param, type, tSeries,
                basePat, cf, baseline, sf);

            // Get The Inflow Object
            if ( errcode == 0 )
            {
                inflow = Node[index].extInflow;
            }
        }
        // Assign new flow rate
        if ( errcode == 0 )
        {
            inflow -> extIfaceInflow = flowrate;
        }
    }
    return(errcode);
}

int DLLEXPORT swmm_setOutfallStage(int index, double stage)
///
/// Input:   index = Index of desired outfall
///          stage = New outfall stage (head)
/// Output:  returns API Error
/// Purpose: Sets new outfall stage and holds until set again.
{
    int errcode = 0;
    // Check if Open
    if (swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if ( index < 0 || index >= Nobjects[NODE] )
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    else if ( Node[index].type != OUTFALL )
    {
        errcode = ERR_API_WRONG_TYPE;
    }
    else
    {
        int k = Node[index].subIndex;
        if ( Outfall[k].type != STAGED_OUTFALL )
        {
            // Change Boundary Conditions Setting Type
            Outfall[k].type = STAGED_OUTFALL;
        }
        Outfall[k].outfallStage = stage / UCF(LENGTH);
    }
    return(errcode);
}

int DLLEXPORT swmm_setGagePrecip(int index, double total_precip)
///
/// Input:   index = Index of desired ID
///          total_precip = rainfall intensity to be set
/// Return:  API Error
/// Purpose: Sets the precipitation in from the external database
{
    int errcode = 0;
    // Check if Open
    if(swmm_IsOpenFlag() == FALSE)
    {
        errcode = ERR_API_INPUTNOTOPEN;
    }
    // Check if object index is within bounds
    else if (index < 0 || index >= Nobjects[GAGE])
    {
        errcode = ERR_API_OBJECT_INDEX;
    }
    // Set the Rainfall rate
    else
    {
        if (Gage[index].dataSource != RAIN_API)
        {
            Gage[index].dataSource = RAIN_API;
        }
        if (Gage[index].isUsed == FALSE)
        {
            Gage[index].isUsed = TRUE;
        }
        if (Gage[index].coGage != -1)
        {
            Gage[index].coGage = -1;
        }
     Gage[index].externalRain = total_precip;
    }
    return(errcode);
}

//-------------------------------
// Utility Functions
//-------------------------------

double* newDoubleArray(int n)
///
///  Warning: Caller must free memory allocated by this function.
///
{
    return (double*) malloc((n)*sizeof(double));
}


void DLLEXPORT freeArray(void** array)
///
/// Helper function used to free array allocated memory by API.
///
{
    if (array != NULL) {
        FREE(*array);
        *array = NULL;
    }
}
