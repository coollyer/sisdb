/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

import ClChart from './chart/cl.chart'
import ClEvent from './event/cl.event'
import ClDatas from './datas/cl.data.js'
import ClFormula from './datas/cl.formula.js'

import * as ChartDef from './cl.chart.def'
import * as DatasDef from './cl.data.def'
import * as Utils from './cl.utils'
import * as DataUtils from './datas/cl.data.tools'

import getValue from './datas/cl.data.tools'

import {
    _initChartInfo
} from './chart/cl.chart.init'

//////////////////////////////////////
/** @module CLAPI */
//////////////////////////////////////
// 进入这里后每个图表仅仅处理固定的数据格式
// 外部必须将数据转换为固定格式后传入
// 统一输出仅仅 ClChart 对象


function getData(source, field, index) {
    return getValue({fields : source.fields, value : source.datas}, field, index)
}

class singleChart {
    constructor(chartInfo, event, chartname, savelinkInfo) {
        this.chartname = chartname
        this.chart = new ClChart(chartInfo)
        this.scale = window.devicePixelRatio
        this.datas = new ClDatas()
        this.chart.initChart(this.datas, event, savelinkInfo)
        this.setTradeTimes(DatasDef.STOCK_TRADETIME)
    }
    clear() {
        this.chart.clear()
        if (typeof this.dataLayer !== 'undefined') {
            this.chart.dataLayer.clearData()
        }
    }
    // 设置交易时间
    setTradeTimes(value) {
        this.chart.setData('TRADETIME', DatasDef.FIELD_TRADETIME, value)
        this.chart.dataLayer.tradeTime = value;
    }
    setTradeDate(value) {
        this.chart.dataLayer.tradeDate = value
    }
    setKeyInfo(value) {
        this.chart.dataLayer.keyInfo = value
    }
    // 得到一个新的一位数组 index = -1 表示遍历所有数据
    getOneDatas(datas, nfield, index)
    {
        let out = []
        if (index === -1) {
            for (let ii = 0; ii < datas.value.length; ii++) {
                out.push(getValue(datas, nfield, ii)) 
            }
        }
        else
        {
            out.push(getValue(datas, nfield, ii)) 
        }
        return out
    }
    // 设置其他数据源
    setData(key, fields, value) {
        this.chart.setData(key, fields, value)
    }
    setAxisXinfo(dataTxt, showTxt) {
        // dataTxt 为正常显示的信息
        // showTxt 为特殊显示的信息 按块定位
        this.chart.dataLayer.axisXinfo.dataTxt = dataTxt
        this.chart.dataLayer.axisXinfo.showTxt = []
        if (showTxt !== undefined) {
            this.chart.dataLayer.axisXinfo.showTxt = showTxt
        }
        this.chart.dataLayer.linkInfo.minIndex = 0
        this.chart.dataLayer.linkInfo.maxIndex = dataTxt.length - 1
        console.log(this.chart.dataLayer.axisXinfo)
    }
    bindData(chart, key) {
        this.chart.bindData(chart, key)
    }
    createChart(curName, className, usercfg, callback) {
        return this.chart.createChart(curName, className, usercfg, callback)
    }
    // initLinkInfo(cname) {

    // }
    setlinkInfo(key, value) {
        this.chart.dataLayer.linkInfo[key] = value
    }
    onPaint(chart) {
        this.chart.onPaint(chart)
    }
}
class manageChart {
    constructor(cfg) {
        this.chartInfo = _initChartInfo(cfg)
        this.event = new ClEvent(this.chartInfo)
        // 这里保存所有linkinfo信息
        this.saveLinkInfo = {}
        // 这里开始初始化
        this.chartList = []
    }
    getChart(cindex) {
        if (cindex < this.chartList.length && cindex >= 0) {
            return this.chartList[cindex]
        }
        return this.chartList[0]
    }
    initChart(cnames) {
        if (cnames.length < 1) {
            return 
        }
        // 保存老的配置
        for (let ii = 0; ii < this.chartList.length; ii++) {
            this.saveLinkInfo[this.chartList[ii].chartname] = Utils.copyJsonOfDeep(this.chartList[ii].chart.dataLayer.linkInfo)
            this.chartList[ii].clear()
            this.chartList[ii] = null
        }
        this.chartList = []
        for (let ii = 0; ii < cnames.length; ii++) {
            this.chartList.push(new singleChart(this.chartInfo, this.event, cnames[ii], this.saveLinkInfo[cnames[ii]]))
        }
        this.event.setCharts(this.chartList)
        return this.chartList[0]
    }
    onPaint(sonChart) {
        for (let ii = 0; ii < this.chartList.length; ii++) {
            this.chartList[ii].onPaint(sonChart)
        }
    }
}
const CH1 = 0.666
const CH2 = 0.334
const CH3 = 0.165

// const CH1 = 0.618
// const CH2 = 0.382 * 0.382
// const CH3 = 0.618 * 0.382

export const CLAPI = {
    CH1,
    CH2,
    CH3,
    ClChart,
    ClEvent,
    ClDatas,
    ClFormula,
    ChartDef,
    DatasDef,
    Utils,
    DataUtils,
    getData,
    manageChart,
}