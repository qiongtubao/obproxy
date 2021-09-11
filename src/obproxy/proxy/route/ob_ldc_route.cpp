/**
 * Copyright (c) 2021 OceanBase
 * OceanBase Database Proxy(ODP) is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#define USING_LOG_PREFIX PROXY

#include "ob_ldc_route.h"

using namespace oceanbase::common;
using namespace oceanbase::share;
using namespace oceanbase::obproxy::obutils;

namespace oceanbase
{
namespace obproxy
{
namespace proxy
{
//ANP, BNP, AMP, BMP;
//ANT, BNT, AMT, BMT;
//CNP, CMP;
//CNT, CMT
static ObRouteType route_order_cursor_of_merge_idc_order[] = {
    ROUTE_TYPE_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANRP, BNRP, AMRP, BMRP, ANWP, BNWP, AMWP, BMWP;
//ANRT, BNRT, AMRT, BMRT, ANWT, BNWT, AMWT, BMWT;
//CNRP, CMRP, CNWP, CMWP
//CNRT, CMRT, CNWT, CMWT
static ObRouteType route_order_cursor_of_readonly_zone_first[] = {
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REMOTE,
    ROUTE_TYPE_MAX
};

//ANRP, BNRP, AMRP, BMRP;
//ANRT, BNRT, AMRT, BMRT;
//CNRP, CMRP
//CNRT, CMRT
static ObRouteType route_order_cursor_of_only_readonly_zone[] = {
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANRP, BNRP, ANWP, BNWP, AMRP, BMRP, AMWP, BMWP;
//ANRT, BNRT, ANWT, BNWT, AMRT, BMRT, AMWT, BMWT;
//CNRP, CNWP, CMRP, CMWP
//CNRT, CNWT, CMRT, CMWT
static ObRouteType route_order_cursor_of_unmerge_zone_first[] = {
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};


//ANWP, BNWP, AMWP, BMWP;
//ANWT, BNWT, AMWT, BMWT;
//CNWP, CMWP
//CNWT, CMWT
static ObRouteType route_order_cursor_of_only_readwrite_zone[] = {
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANP, BNP, AMP, BMP;
//CNP, CMP;
//ANT, BNT, AMT, BMT;
//CNT, CMT
static ObRouteType route_order_cursor_of_merge_idc_order_optimized[] = {
    ROUTE_TYPE_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_MERGE_REGION,

    ROUTE_TYPE_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANRP, BNRP, AMRP, BMRP, ANWP, BNWP, AMWP, BMWP;
//CNRP, CMRP, CNWP, CMWP
//ANRT, BNRT, AMRT, BMRT, ANWT, BNWT, AMWT, BMWT;
//CNRT, CMRT, CNWT, CMWT
static ObRouteType route_order_cursor_of_readonly_zone_first_optimized[] = {
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REMOTE,
    ROUTE_TYPE_MAX
};

//ANRP, BNRP, AMRP, BMRP;
//CNRP, CMRP
//ANRT, BNRT, AMRT, BMRT;
//CNRT, CMRT
static ObRouteType route_order_cursor_of_only_readonly_zone_optimized[] = {
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANRP, BNRP, ANWP, BNWP, AMRP, BMRP, AMWP, BMWP;
//CNRP, CNWP, CMRP, CMWP
//ANRT, BNRT, ANWT, BNWT, AMRT, BMRT, AMWT, BMWT;
//CNRT, CNWT, CMRT, CMWT
static ObRouteType route_order_cursor_of_unmerge_zone_first_optimized[] = {
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READONLY_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READONLY_MERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};


//ANWP, BNWP, AMWP, BMWP;
//CNWP, CMWP
//ANWT, BNWT, AMWT, BMWT;
//CNWT, CMWT
static ObRouteType route_order_cursor_of_only_readwrite_zone_optimized[] = {
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_PARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_READWRITE_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_READWRITE_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANPF, BNPF, AMPF, BMPF;
//ANPL, BNPL, AMPL, BMPL;
//ANT, BNT, AMT, BMT;
//CNPF, CMPF;
//CNPL, CMPL;
//CNT, CMT
static ObRouteType route_order_cursor_of_follower_first[] = {
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANPF, BNPF, ANPL, BNPL;
//AMPF, BMPF, AMPL, BMPL;
//ANT, BNT, AMT, BMT;
//CNPF, CNPL;
//CMPF, CMPL;
//CNT, CMT
static ObRouteType route_order_cursor_of_unmerge_follower_first[] = {
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REGION,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REMOTE,

    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REMOTE,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};


//ANPF, BNPF, AMPF, BMPF;
//ANPL, BNPL, AMPL, BMPL;
//CNPF, CMPF;
//CNPL, CMPL;
//ANT, BNT, AMT, BMT;
//CNT, CMT
static ObRouteType route_order_cursor_of_follower_first_optimized[] = {
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANPF, BNPF, ANPL, BNPL;
//AMPF, BMPF, AMPL, BMPL;
//CNPF, CNPL;
//CMPF, CMPL;
//ANT, BNT, AMT, BMT;
//CNT, CMT
static ObRouteType route_order_cursor_of_unmerge_follower_first_optimized[] = {
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REGION,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_LEADER_PARTITION_UNMERGE_REMOTE,

    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REMOTE,
    ROUTE_TYPE_LEADER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANPD, BNPD, AMPD, BMPD
//CNPD, CMPD
//ANP, BNP, AMP, BMP;
//CNP, CMP;
//ANT, BNT, AMT, BMT;
//CNT, CMT
static ObRouteType route_order_cursor_of_dup_strong_read_order[] = {
    ROUTE_TYPE_DUP_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_DUP_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_DUP_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_DUP_PARTITION_MERGE_REGION,
    ROUTE_TYPE_DUP_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_DUP_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_PARTITION_MERGE_REGION,

    ROUTE_TYPE_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANPF, BNPF, AMPF, BMPF;
//ANT, BNT, AMT, BMT;
//CNPF, CMPF;
//CNT, CMT
static ObRouteType route_order_cursor_of_follower_only[] = {
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

//ANPF, BNPF, AMPF, BMPF;
//CNPF, CMPF;
//ANT, BNT, AMT, BMT;
//CNT, CMT
static ObRouteType route_order_cursor_of_follower_only_optimized[] = {
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REGION,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_LOCAL,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REGION,

    ROUTE_TYPE_FOLLOWER_PARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_FOLLOWER_PARTITION_MERGE_REMOTE,

    ROUTE_TYPE_NONPARTITION_UNMERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_UNMERGE_REGION,
    ROUTE_TYPE_NONPARTITION_MERGE_LOCAL,
    ROUTE_TYPE_NONPARTITION_MERGE_REGION,

    ROUTE_TYPE_NONPARTITION_UNMERGE_REMOTE,
    ROUTE_TYPE_NONPARTITION_MERGE_REMOTE,

    ROUTE_TYPE_MAX
};

const ObRouteType *ObLDCRoute::route_order_cursor_[] = {
    route_order_cursor_of_merge_idc_order,
    route_order_cursor_of_readonly_zone_first,
    route_order_cursor_of_only_readonly_zone,
    route_order_cursor_of_unmerge_zone_first,
    route_order_cursor_of_only_readwrite_zone,
    route_order_cursor_of_merge_idc_order_optimized,
    route_order_cursor_of_readonly_zone_first_optimized,
    route_order_cursor_of_only_readonly_zone_optimized,
    route_order_cursor_of_unmerge_zone_first_optimized,
    route_order_cursor_of_only_readwrite_zone_optimized,
    route_order_cursor_of_follower_first,
    route_order_cursor_of_unmerge_follower_first,
    route_order_cursor_of_follower_first_optimized,
    route_order_cursor_of_unmerge_follower_first_optimized,
    route_order_cursor_of_dup_strong_read_order,
    route_order_cursor_of_follower_only,
    route_order_cursor_of_follower_only_optimized,
};

int64_t ObLDCRoute::route_order_size_[] = {
    sizeof(route_order_cursor_of_merge_idc_order) / sizeof(ObRouteType),//13
    sizeof(route_order_cursor_of_readonly_zone_first) / sizeof(ObRouteType),//25
    sizeof(route_order_cursor_of_only_readonly_zone) / sizeof(ObRouteType),//13
    sizeof(route_order_cursor_of_unmerge_zone_first) / sizeof(ObRouteType),//25
    sizeof(route_order_cursor_of_only_readwrite_zone) / sizeof(ObRouteType),//13
    sizeof(route_order_cursor_of_merge_idc_order_optimized) / sizeof(ObRouteType),//13
    sizeof(route_order_cursor_of_readonly_zone_first_optimized) / sizeof(ObRouteType),//25
    sizeof(route_order_cursor_of_only_readonly_zone_optimized) / sizeof(ObRouteType),//13
    sizeof(route_order_cursor_of_unmerge_zone_first_optimized) / sizeof(ObRouteType),//25
    sizeof(route_order_cursor_of_only_readwrite_zone_optimized) / sizeof(ObRouteType),//13

    sizeof(route_order_cursor_of_follower_first) / sizeof(ObRouteType),//19
    sizeof(route_order_cursor_of_unmerge_follower_first) / sizeof(ObRouteType),//19
    sizeof(route_order_cursor_of_follower_first_optimized) / sizeof(ObRouteType),//19
    sizeof(route_order_cursor_of_unmerge_follower_first_optimized) / sizeof(ObRouteType),//19

    sizeof(route_order_cursor_of_dup_strong_read_order) / sizeof(ObRouteType),//18

    sizeof(route_order_cursor_of_follower_only) / sizeof(ObRouteType),//13
    sizeof(route_order_cursor_of_follower_only_optimized) / sizeof(ObRouteType),//13
};

const ObLDCItem *ObLDCRoute::get_next_item()
{
  ObLDCItem *ret_item = NULL;
  if (!location_.is_empty()) {
    const int64_t *site_start_index_array = location_.get_site_start_index_array();
    ObLDCItem *item_array = location_.get_item_array();
    ObRouteType route_type = get_route_type(curr_cursor_index_);
    ObIDCType idc_type = get_idc_type(route_type);
    bool need_break = (ROUTE_TYPE_MAX == route_type);
    while (!need_break) {
      if (next_index_in_site_ >= site_start_index_array[idc_type + 1]) {
        LOG_DEBUG("need try next cursor type", K_(curr_cursor_index),
                  "curr_route_type", get_route_type_string(route_type),
                  K_(next_index_in_site), "site_start_index_array",
                  ObArrayWrap<int64_t>(site_start_index_array, MAX_IDC_TYPE + 1));
        ++curr_cursor_index_;
        route_type = get_route_type(curr_cursor_index_);
        if (ROUTE_TYPE_MAX == route_type) {
          LOG_DEBUG("it is reach end now");
          need_break = true;
        } else {
          idc_type = get_idc_type(route_type);
          next_index_in_site_ = site_start_index_array[idc_type];
        }
      } else {
        ret_item = item_array + next_index_in_site_;
        ++next_index_in_site_;
        if (!ret_item->is_used_
            && is_same_role(route_type, *ret_item)
            && is_same_partition_type(route_type, *ret_item)
            && is_same_zone_type(route_type, *ret_item)
            && is_same_dup_replica_type(route_type, *ret_item)
            && (disable_merge_status_check_ || is_same_merge_type(route_type, *ret_item))) {
          ret_item->is_used_ = true;
          need_break = true;
          LOG_DEBUG("succ to get_next_replica", KPC(ret_item), K_(disable_merge_status_check),
                    "curr_route_type", get_route_type_string(route_type));
        } else {
          LOG_DEBUG("item is not excepted, try next", KPC(ret_item),
                    "curr_route_type", get_route_type_string(route_type),
                    K_(disable_merge_status_check),
                    K_(curr_cursor_index), K_(next_index_in_site));
          ret_item = NULL;
        }
      }
    }
  } else {
    //set to max idx
    curr_cursor_index_ = route_order_size_[policy_] - 1;
  }
  return ret_item;
}
} // end of namespace proxy
} // end of namespace obproxy
} // end of namespace oceanbase
