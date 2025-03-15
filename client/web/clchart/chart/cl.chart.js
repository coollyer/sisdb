/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// Following is the entity definition of ClChart
// generally only use this class to implement the function
// A ClChart class is just a drawing of the chart belonging to the dispatch framework, mouse and keyboard events, and does not store data
// When multiple maps are linked, the subgraphs are all here to get relevant information.

import {
    copyJsonOfDeep,
    setJsonOfDeep,
} from '../cl.utils'
import ClEvent from '../event/cl.event'
import {
    bindEvent
} from '../event/cl.event.handler'
// import ClChartLine from './cl.chart.line'
// import ClChartBoard from './cl.chart.board'
import { setColor, setStandard, _createClass } from './cl.chart.init'
import { _clearRect } from './cl.draw'

/**
 * Class representing ClChart
 * @export
 * @class ClChart
 */
export default class ClChart {
    /**
  
     * Creates an instance of ClChart.
     * @param {Object} syscfg
     */
    constructor(syscfg) {
        this.mainCanvas = syscfg.mainCanvas
        this.context = this.mainCanvas.context
        this.moveCanvas = syscfg.moveCanvas
        this.sysColor = syscfg.sysColor
        // Use this to determine if it is the root element
        this.father = undefined
    }
    /**
     * Re-initialize Chart to clean up all charts and data
     * @param {any} dataLayer data layer
     * @param {any} eventLayer event layer
     * @memberof ClChart
     */
    // 所有事件由外部获取事件后传递到eventLayer后，再统一分发给相应的图表
    // 这里保存的 eventLayer 主要用于当鼠标位于主图时 需要联动其他相关子图时传递事件使用
     initChart(dataLayer, eventLayer, savelinkInfo) {
        // this.checkConfig();
        // 初始化子chart为空
        this.childCharts = {}
        // 设置数据层，同时对外提供设置接口
        this.dataLayer = dataLayer
        this.dataLayer.father = this
        if (savelinkInfo !== undefined) {
            this.dataLayer.linkInfo = copyJsonOfDeep(savelinkInfo)
        }
        // console.log(JSON.stringify(this.dataLayer.linkInfo));
         // 设置事件层，同时对外提供设置接口
        this.eventLayer = eventLayer
    }
    /**
     * clear current data and recharge linkInfo
     *
     * @memberof ClChart
     */
    clear() {
        this.fastDraw = false
        this.dataLayer.clearData()
        this.childCharts = {}
    }
    // /**
    //  * clear current data and recharge linkInfo
    //  *
    //  * @memberof ClChart
    //  */
    // clearLayout() {

    //     _clearRect(this.moveCanvas.context, 0, 0,
    //         this.moveCanvas.canvas.width, this.moveCanvas.canvas.height)

    //     this.childCharts = {}
    //     this.fastDraw = false
    //     console.log(this.dataLayer)
    //     if (typeof this.dataLayer !== 'undefined')
    //     {
    //         this.dataLayer.clearData()
    //     }
    //     this.linkInfo = copyJsonOfDeep(DEFAULT_LINKINFO)
    // }
    /**
     * get child chart by key
     * @param {String} key child chart key
     * @return {Object} child chart
     * @memberof ClChart
     */
    getChart(key) {
        return this.childCharts[key]
    }
    /**
     * get data layer
     * @return {Object} data layer
     * @memberof ClChart
     */
    getDataLayer() {
        return this.dataLayer
    }
    /**
     * set data layer
     * @param {Object} layer
     * @memberof ClChart
     */
    
    /**
     * Set the corresponding basic data key of the chart
     * @param {Object} chart
     * @param {String} key
     * @memberof ClChart
     */
    bindData(chart, key) {
        if (chart.hotKey !== key) {
            this.dataLayer.linkInfo.showMode = 'init' // 切换数据后需要重新画图
            this.dataLayer.linkInfo.minIndex = -1
            chart.hotKey = key // hotKey 针对chart的数据都调用该数据源
            this.fastDrawEnd() // 热点数据改变，就重新计算
        }
    }
    /**
     * set data
     * @param {String} key data key
     * @param {Object} fields data field definition
     * @param {any} value
     * @memberof ClChart
     */
    setData(key, fields, value) {
        let info = value
        if (typeof value === 'string') info = JSON.parse(value)
        this.dataLayer.setData(key, fields, info)
        this.fastDrawEnd() // 新的数据被设置，就重新计算
    }
    /**
     * get data from datalayer by key
     * @param {String} key   
     * @return {Array}
     * @memberof ClChart
     */
    getData(key) {
        if (this.fastDraw) {
            if (this.fastBuffer[key] !== undefined) {
                return this.fastBuffer[key]
            }
        }
        const out = this.dataLayer.getData(key, this.dataLayer.linkInfo.rightMode)
        if (this.fastDraw) {
            this.fastBuffer[key] = out
        }
        return out
    }
    /**
     * init line data
     * @param {Array} data
     * @param {Array} lines
     * @memberof ClChart
     */
    readyData(data, lines) {
        for (let k = 0; k < lines.length; k++) {
            if (lines[k].formula === undefined) continue
            if (!this.fastDraw || (this.fastDraw && this.fastBuffer[lines[k].formula.key] === undefined)) {
                this.dataLayer.makeLineData(
                    { data, minIndex: this.dataLayer.linkInfo.minIndex, maxIndex: this.dataLayer.linkInfo.maxIndex },
                    lines[k].formula.key,
                    lines[k].formula.command
                )
            }
        }
    }
    /**
     * create chart
     * @param {String} name name is a unique name, the same name will override the previous class with the same name
     * @param {String} className class name
     * @param {Object} usercfg user custom config
     * @param {Function} callback callback
     * @return {Object} chart instance
     * @memberof ClChart
     */
    createChart(name, className, usercfg, callback) {
        let chart = _createClass(className, this);
        console.log(chart);

        chart.name = name
        this.childCharts[name] = chart

        chart.init(usercfg, callback) // 根据用户配置初始化信息框

        return chart
    }
    /**
     * draw all the layers contained in this area
     * @param {Object} chart
     * @memberof ClChart
     */
    onPaint(chart) {
        if (typeof this.context._beforePaint === 'function') {
            this.context._beforePaint()
        }
        this.fastDrawBegin()
        for (const key in this.childCharts) {
            console.log('onPaint', key, this.childCharts)
            if (chart !== undefined) {
                if (this.childCharts[key] === chart) {
                    this.childCharts[key].onPaint()
                }
            } else {
                this.childCharts[key].onPaint()
            }
        }
        // this.fastDrawEnd();
        if (typeof this.context._afterPaint === 'function') {
            this.context._afterPaint()
        }
    }
    /**
     * Used for the same group of multi-graph only take data once, this can speed up the display, the program structure will not be chaotic
     * @memberof ClChart
     */
    fastDrawBegin() {
        if (!this.fastDraw) {
            this.fastBuffer = []
            this.fastDraw = true
        }
    }
    /**
     * set whether to turn on quick drawing
     * @memberof ClChart
     */
    fastDrawEnd() {
        this.fastDraw = false
    }

    // /**
    //  * Set whether to hide the information bar
    //  * @param {String} isHideInfo
    //  * @memberof ClChart
    //  */
    // setHideInfo(showCursorInfo) {
    //     if (showCursorInfo === undefined) return
    //     if (showCursorInfo !== this.dataLayer.linkInfo.showCursorInfo) {
    //         this.dataLayer.linkInfo.showCursorInfo = showCursorInfo
    //         this.onPaint()
    //     }
    // }
    /**
     * Set the background color
     * @param {String} sysColor
     * @param {Object | null} chart
     * @memberof ClChart
     */
    setColor(sysColor, chart) {
        this.color = setColor(sysColor)
        if (chart === undefined) chart = this
        for (const key in chart.childCharts) {
            chart.childCharts[key].color = this.color
            this.setColor(sysColor, chart.childCharts[key])
        }
        // 需要将其子配置的颜色也一起改掉
        for (const key in chart.childDraws) {
            chart.childDraws[key].color = this.color
            this.setColor(sysColor, chart.childDraws[key])
        }
        for (const key in chart.childLines) {
            chart.childLines[key].color = this.color
            this.setColor(sysColor, chart.childLines[key])
        }
        this.sysColor = sysColor
    }
    /**
     * setting drafting standards
     * @param {String} standard
     * @memberof ClChart
     */
    setStandard(standard) {
        setStandard(standard)
        this.setColor(this.sysColor)
        this.onPaint()
    }
    // this.makeLineData = function(data, key, formula, punit) {
    //   return this.dataLayer.makeLineData(data, key, this.dataLayer.linkInfo.minIndex, this.linkInfo.maxIndex, formula);
    // }
}
