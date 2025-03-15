/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 所有股票相关的数据定义在这里
// clchart 只关心根据标准输入数据画图

// 数据类型和数据名称分开；根据数据类型生成
// 在comm中定义几个函数，用户可以自己定义数据类型，自己定义按钮，自己定义画线的方式，
// 只要用户产生的类支持通用的接口onpaint等函数，并成功注册进来，就可以被chart拿来画图了
// 这样扩展起来才方便
// 而chart.line只画基础的一些图形，其他非标的图形由用户自行定义，包括数据生成的算法

export const STOCK_TYPE_IDX = 0
export const STOCK_TYPE_STK = 1

//////////////////////////////////////////
// 特别注意 以下的数据字段定义不能修改名称 
//////////////////////////////////////////
export const FIELD_TRADETIME = {
    open: 0,
    stop: 1
}
export const STOCK_TRADETIME = [[930, 1130], [1300, 1500]]

// 股票信息定义
export const FIELD_STKINFO = {
    date:     0,  // 当前日期
    marker:   1,
    code:     2,
    name:     3,  // 股票名称
    search:   4,  // 拼音检索
    style:    5,  // 股票类型
    coindot:  6,  // 保留小数点
    prcunit:  7,  // 价格单位
    volunit:  8,  // 量单位
    mnyunit:  9,  // 额单位
    agop:     10, // 前收盘
    stopmax:  11, // 涨停价
    stopmin:  12, // 跌停价
}

// 这里保存历史除权和财务等信息
export const FIELD_FRIGHT = {
    date:     0,  // 当前日期
    fright:   1,  // 除权因子
    wstake:   2,  // 全部股本
    fstake:   3,  // 流通股本
    income:   4,  // 每股收益 用于计算市盈率
}
// 特殊Kbar的定义
export const FIELD_KBAR = {
    date:  0,
    open:  1,
    maxp:  2,
    minp:  3,
    newp:  4,
}
// 两种单线的定义
export const FIELD_LINE = {
    time:  0,
    value: 1
}
export const FIELD_ILINE = {
    index: 0,
    value: 1
}
// 所有数据默认增加一个 index 字段
// obj['index'] = Object.keys(obj).length
// 或者建立一个一维索引数组来管理 

/////////////////////////////////////////////////////////
// 特别注意 以上的数据字段定义不能修改名称 内部都以名称来获取数据的
/////////////////////////////////////////////////////////

// 以下是预定义的默认数据结构
export const FIELD_DATE = {
    date: 0,
    open: 1,
    maxp: 2,
    minp: 3,
    newp: 4,
    sumv: 5,
    summ: 6
}
export const FIELD_MINU = {
    msec:  0, // 按交易时间的分钟索引
    open:  1,
    maxp:  2,
    minp:  3,
    newp:  4,
    sumv:  5,
    summ:  6,
    curv:  7,
    curm:  8,
    index: 9,
}

export const FIELD_TICK = {
    msec: 0,
    newp: 1,
    sumv: 2
}
export const FIELD_MDAY = {
    msec: 0,
    newp: 1,
    curv: 2,  // 当前量
    sumv: 4,  // 合计的成交量
    summ: 5   // 合计的成交金额
}

export const FIELD_SNAP = {
    msec: 0,
    open: 1,
    maxp: 2,
    minp: 3,
    newp: 4,
    sumv: 5,
    summ: 6,
    askp0: 15,
    askp1: 16,
    askp2: 17,
    askp3: 18,
    askp4: 19,
    askp5: 20,
    askp6: 21,
    askp7: 22,
    askp8: 23,
    askp9: 24,
    askv0: 25,
    askv1: 26,
    askv2: 27,
    askv3: 28,
    askv4: 29,
    askv5: 30,
    askv6: 31,
    askv7: 31,
    askv8: 33,
    askv9: 34,
    bidp0: 35,
    bidp1: 36,
    bidp2: 37,
    bidp3: 38,
    bidp4: 39,
    bidp5: 40,
    bidp6: 41,
    bidp7: 41,
    bidp8: 43,
    bidp9: 44,
    bidv0: 45,
    bidv1: 46,
    bidv2: 47,
    bidv3: 48,
    bidv4: 49,
    bidv5: 50,
    bidv6: 51,
    bidv7: 51,
    bidv8: 53,
    bidv9: 54
}
// 指数实时行情
export const FIELD_SNAP_IDX = {
    msec: 0,
    agop: 1,
    open: 2,
    maxp: 3,
    minp: 4,
    newp: 5,
    sumv: 6,
    summ: 7,
    newpups : 8,
    newpdns : 9,
    agopups : 10,
    agopdns : 11,
    openups : 12,
    opendns : 13
}

// 交易信息定义
export const FIELD_ADJUST = {
    msec: 0, // 交易时间
    code: 1, // 股票代码
    flag: 2, // B S 买卖标志
    newp: 3, // 调仓参考价格 - 用于计算模拟收益
    svol: 4, // 需要的成交量
}

export const FIELD_REPORT = {
    send_msec: 0, // 发送时间
    recv_msec: 1, // 收到时间
    code:      2, // 股票代码
    status:    3, // 委托状态
    bsflag:    4, // B S 买卖标志
    wtnewp:    5, // 委托价格
    cjnewp:    6, // 成交价格
    wt_vol:    7, // 委托量
    cj_vol:    8, // 成交量
    cx_vol:    9, // 撤销量
    summny:   10, // 成交金额
    feemny:   11, // 手续费
    info:     12  // 交易参数
}
