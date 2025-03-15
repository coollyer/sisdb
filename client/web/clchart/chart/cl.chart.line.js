/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是ClChartLine的实体定义  --- 画线类

// 实际上就是获取某种类型数据，然后横坐标 0 开始排序，永远是数值型，但显示出什么要到对应的数组中找
// 纵坐标根据数据类型，计算最大最小值，然后根据画线类型画出数据线，所有的不同全部在ClChart中处理

import {
    _fillRect,
    _drawRect,
    _setLineWidth,
    _getTxtWidth,
    _drawBegin,
    _drawHline,
    _drawEnd,
} from './cl.draw'
import {
    setFixedLineFlags,
    setMoveLineFlags
} from './cl.chart.tools'
import {
    initCommonInfo,
    checkLayout,
    getLineColor,
} from './cl.chart.init'
import {
    CHART_LAYOUT
} from '../cl.chart.def'
import {
    updateJsonOfDeep,
    copyJsonOfDeep,
    offsetRect,
    inArray,
    isEmptyArray,
    formatShowTime,
    formatInfo,
    inRangeX
} from '../cl.utils'

import {
    STOCK_TYPE_IDX
} from '../cl.data.def'
import getValue, {
    getSize,
    getValueFidx
} from '../datas/cl.data.tools'

import ClChartButton from './cl.chart.button'
import ClChartScroll from './cl.chart.scroll'
import ClDrawAxisX from './cl.draw.axisX'
import ClDrawAxisY from './cl.draw.axisY'
import ClDrawCursor from './cl.draw.cursor'
import ClDrawGrid from './cl.draw.grid'
import ClDrawInfo from './cl.draw.info'
import ClDrawLine from './cl.draw.line'
import ClDrawVLine from './cl.draw.vline'

/**
 * Class representing ClChartLine
 * @export
 * @class ClChartLine
 */
export default class ClChartLine {
    /**

     * Creates an instance of ClChartLine.
     * @param {Object} father line chart's parent context
     */
    constructor(father) {
        initCommonInfo(this, father)

        this.dataLayer = father.dataLayer
        this.linkInfo = this.dataLayer.linkInfo

        this.static = this.dataLayer.keyInfo[this.linkInfo.curKey].static

        this.data = {}
        this.axisYdata = {}
    }
    /**
     * init line
     * @param {Object} cfg line's config
     * @param {any} callback initialized callback
     * @memberof ClChartLine
     */
    init(cfg, callback) {
        this.callback = callback
        this.rectMain = cfg.rectMain || {
            left: 0,
            top: 0,
            width: 400,
            height: 200
        }

        this.layout = updateJsonOfDeep(cfg.layout, CHART_LAYOUT)
        this.config = copyJsonOfDeep(cfg.config)
        // 这里直接赋值是因为外部已经设置好了配置才会开始初始化
        this.buttons = cfg.config.buttons || []
        // console.log(cfg, this.buttons);
        this.scroll = cfg.config.scroll || {
            display: 'none'
        }
        this.childCharts = {}

        // 下面对配置做一定的校验
        this.checkConfig()
        // 再做一些初始化运算，下面的运算范围是初始化设置后基本不变的数据
        this.setPublicRect()
        // 初始化子窗口
        this.childDraws = {}
        this.setChildDraw()
        this.childLines = {}
        this.setChildLines()
        // 初始化按钮
        this.createButtons()
        // 初始化滚动条
        this.setScroll()
    }
    /**
     * check config
     * @memberof ClChartLine
     */
    checkConfig() { // 检查配置有冲突的修正过来
        checkLayout(this.layout)
        if (this.config.zoomInfo !== undefined) {
            this.setZoomInfo()
        }
    }
    /**
     * set public rect
     * @memberof ClChartLine
     */
    setPublicRect() { // 计算所有矩形区域
        // rectChart 画图区
        // rectTitle rectMess
        // rectAxisX
        // rectScroll
        // rectAxisYLeft rectAxisYRight
        // rectGridLine 坐标线区域

        this.rectChart = offsetRect(this.rectMain, this.layout.margin)
        const axisInfo = {
            width: this.layout.axisX.width
        } //

        // 计算title和mess矩形位置
        this.rectTitle = {
            left: this.rectMain.left,
            top: this.rectMain.top,
            width: 0,
            height: 0
        }
        this.rectMess = {
            left: this.rectMain.left,
            top: this.rectMain.top,
            width: 0,
            height: 0
        }
        this.rectButton = {
            left: this.rectMain.left,
            top: this.rectMain.top,
            width: 0,
            height: 0
        }
        if (this.config.title.display !== 'none') {
            this.rectTitle = {
                left: this.rectChart.left,
                top: this.rectChart.top,
                width: axisInfo.width,
                height: this.layout.title.height
            }
            const rightW = 2 * this.scale
            if (this.buttons.length > 0) {
                const btnh = this.layout.title.height - 4 * this.scale
                const btnw = this.buttons.length * btnh + 4 * this.scale
                this.rectButton = {
                    left: this.rectChart.left + this.rectChart.width - btnw - rightW,
                    top: this.rectChart.top + 2 * this.scale,
                    width: btnw,
                    height: btnh
                }
            }
            this.rectMess = {
                left: this.rectChart.left + this.rectTitle.width + this.scale,
                top: this.rectChart.top,
                width: this.rectChart.width - this.rectTitle.width - this.rectButton.width - rightW - this.scale,
                height: this.layout.title.height
            }

        }

        // 计算rectAxisX和rectScroll矩形位置
        axisInfo.left = this.rectChart.left
        axisInfo.right = this.rectChart.left + this.rectChart.width
        axisInfo.top = this.rectTitle.top + this.rectTitle.height
        axisInfo.bottom = this.rectChart.top + this.rectChart.height

        if (this.axisPlatform !== 'phone') {
            if (this.config.axisY.left.display !== 'none') axisInfo.left += this.layout.axisX.width
            if (this.config.axisY.right.display !== 'none') axisInfo.right -= this.layout.axisX.width
        }
        if (this.config.axisX.display !== 'none') {
            axisInfo.bottom -= this.layout.axisX.height
        }
        if (this.scroll.display !== 'none') {
            axisInfo.bottom -= this.layout.scroll.size
        }
        // 此时已经可以得出画坐标线的区域了
        this.rectGridLine = {
            left: axisInfo.left,
            top: axisInfo.top,
            width: axisInfo.right - axisInfo.left,
            height: axisInfo.bottom - axisInfo.top
        }
        this.rectAxisYLeft = {
            left: this.rectChart.left,
            top: axisInfo.top,
            width: axisInfo.width,
            height: axisInfo.bottom - axisInfo.top
        }
        this.rectAxisYRight = {
            left: this.rectChart.left + this.rectChart.width - axisInfo.width,
            top: axisInfo.top,
            width: axisInfo.width,
            height: axisInfo.bottom - axisInfo.top
        }

        this.rectChart = offsetRect(this.rectGridLine, this.layout.offset)

        this.rectAxisX = {
            left: 0,
            top: axisInfo.bottom,
            width: 0,
            height: 0
        }
        if (this.config.axisX.display !== 'none') {
            this.rectAxisX = {
                left: axisInfo.left,
                top: axisInfo.bottom + this.scale,
                width: axisInfo.right - axisInfo.left,
                height: this.layout.axisX.height
            }
        }
        if (this.scroll.display !== 'none') {
            this.rectScroll = {
                left: axisInfo.left,
                // top: this.rectAxisX.top + this.rectAxisX.height + this.scale,
                top: this.rectAxisX.top + this.rectAxisX.height,
                width: axisInfo.right - axisInfo.left,
                height: this.layout.scroll.size
            }
        }
    }
    /**
     * get line by data key
     * @param {String} line
     * @return {Object} line instance
     * @memberof ClChartLine
     */
    getLineDataKey(line) {
        if (line.formula === undefined) return this.hotKey
        return line.formula.key
    }
    /**
     * set child line instance
     * @memberof ClChartLine
     */
    setChildLines() {
        // l_kbar，l_line，l_nowvol，l_vbar l_nowline
        let line
        let clr = 0
        for (let i = 0; i < this.config.lines.length; i++) {
            const ClassName = this.config.lines[i].className
            line = new ClassName(this, this.rectChart)

            this.childLines['NAME' + i] = line
            line.name = 'NAME' + i
            line.hotKey = this.getLineDataKey(this.config.lines[i])
            if (this.config.lines[i].info === undefined) {
                line.info = {
                    labelX: 'index',
                    labelY: 'value'
                }
            } else {
                line.info = this.config.lines[i].info
            }
            line.info.colorIndex = clr
            clr++
        }
    }
    /**
     * set child draw instance
     * @memberof ClChartLine
     */
    setChildDraw() {
        let draw
        draw = new ClDrawCursor(this, {
            rectMain: this.rectGridLine,
            rectChart: this.rectChart,
            format: 'cross'
        })
        this.childDraws['CURSOR'] = draw

        if (this.config.title.display !== 'none') {
            draw = new ClDrawInfo(this, this.rectTitle, this.rectMess)
            this.childDraws['TITLE'] = draw
        }
        if (this.config.axisY.left.display !== 'none') {
            draw = new ClDrawAxisY(this, this.rectAxisYLeft, 'left')
            this.childDraws['AXISY_LEFT'] = draw
        }
        if (this.config.axisY.right.display !== 'none') {
            draw = new ClDrawAxisY(this, this.rectAxisYRight, 'right')
            this.childDraws['AXISY_RIGHT'] = draw
        }
        if (this.config.axisX.display !== 'none') {
            draw = new ClDrawAxisX(this, this.rectAxisX)
            this.childDraws['AXISX'] = draw
        }
        draw = new ClDrawGrid(this, this.rectGridLine)
        this.childDraws['GRID'] = draw
        // 下面是line的定义
    }
    /**
     * set scroll instance
     * @memberof ClChartLine
     */
    setScroll() {
        if (this.scroll.display === 'none') return
        const chart = new ClChartScroll(this)
        chart.name = 'HSCROLL'
        chart.father = this
        this.childCharts[chart.name] = chart

        chart.init({
                rectMain: this.rectScroll,
                config: {
                    width: 8
                }
            },
            result => {
                // console.log(this, this.linkInfo);
                if (this.linkInfo.minIndex !== result.minIndex) {
                    this.linkInfo.minIndex = result.minIndex
                    this.linkInfo.showMode = 'move'
                    this.father.fastDrawEnd()
                    this.father.onPaint()
                }
            })
    }
    /**
     * create button
     * @param {String} name button name
     * @return {Object} button instacne
     * @memberof ClChartLine
     */
    setButton(chart, config, rect) {
        switch (config.clickName) {
            case 'zoomin':
                chart.init({
                    rectMain: rect,
                    info: config,
                },
                ( /* result */ ) => {
                    if (this.linkInfo.zoomIndex > 0) {
                        this.linkInfo.zoomIndex--
                        this.setZoomInfo()
                        this.father.onPaint()
                    }
                })
                break;
            case 'zoomout':
                chart.init({
                    rectMain: rect,
                    info: config,
                },
                ( /* result */ ) => {
                    if (this.linkInfo.zoomIndex < this.config.zoomInfo.list.length - 1) {
                        this.linkInfo.zoomIndex++
                        this.setZoomInfo()
                        this.father.onPaint()
                    }
                })
                break;
            case 'exright':
                chart.init({
                    rectMain: rect,
                    info: config,
                },
                result => {
                    if (this.linkInfo.rightMode !== result.info.selectVar) {
                        this.linkInfo.rightMode = result.info.selectVar
                        this.father.fastDrawEnd()
                        this.father.onPaint()
                    }
                })
                chart.info.selectVar = this.linkInfo.rightMode
                break;
            default: 
                chart.init({
                    rectMain: rect,
                    info: config,
                },
                result => {
                    // if (this.linkInfo.rightMode !== result.info.selectVar) {
                    //     this.linkInfo.rightMode = result.info.selectVar
                    //     this.father.fastDrawEnd()
                    //     this.father.onPaint()
                    // }
                })
                // chart.info.selectVar = this.linkInfo.rightMode
                break;
        }
    }
    /**
     * set buttons
     * @memberof ClChartLine
     */
    createButtons() {
        let chart
        let yy = this.rectButton.top
        let ww = this.rectButton.width / this.buttons.length
        let xx = this.rectButton.left
        for (let index = 0; index < this.buttons.length; index++) {
            
            const bname = this.buttons[index].clickName
            // console.log(this.childCharts[bname]);
            if (this.childCharts[bname] !== undefined) {
                continue
            }
            chart = new ClChartButton(this)
            chart.name = bname
            this.childCharts[bname] = chart

            this.setButton(chart, this.buttons[index], {
                left: xx,
                top: yy,
                width: ww,
                height: this.rectButton.height
            })
            xx += ww
        }
        // console.log(this.buttons.length, this.childCharts);
    }
    /**
     * set zoom info
     * @memberof ClChartLine
     */
    setZoomInfo() {
        if (this.linkInfo.zoomIndex === -1) {
            this.linkInfo.zoomIndex = this.config.zoomInfo.index
        }
        this.linkInfo.zoomIndex = this.linkInfo.zoomIndex > this.config.zoomInfo.list.length - 1 ? this.config.zoomInfo.list.length - 1 : this.linkInfo.zoomIndex
        this.linkInfo.zoomIndex = this.linkInfo.zoomIndex < 0 ? 0 : this.linkInfo.zoomIndex
        const value = this.config.zoomInfo.list[this.linkInfo.zoomIndex]

        this.linkInfo.unitX = value * this.scale
        // this.linkInfo.unitX = Math.max(this.linkInfo.unitX, 1)
        // if (this.linkInfo.unitX < this.scale) this.linkInfo.unitX = this.scale
        this.linkInfo.spaceX = this.linkInfo.unitX / 4
        // this.linkInfo.spaceX = Math.max(this.linkInfo.spaceX, 1)
        // if (this.linkInfo.spaceX < this.scale) this.linkInfo.spaceX = this.scale
        
        // 设置状态
        // if (this.childCharts['zoomin']) {
        //     if (this.linkInfo.zoomIndex === 0) this.childCharts['zoomin'].setStatus('disabled')
        //     else this.childCharts['zoomin'].setStatus('enabled')
        // }
        // if (this.childCharts['zoomout']) {
        //     if (this.linkInfo.zoomIndex === this.config.zoomInfo.list.length - 1) this.childCharts['zoomout'].setStatus('disabled')
        //     else this.childCharts['zoomout'].setStatus('enabled')
        // }
        // console.log('zoom',  JSON.stringify(this.dataLayer.linkInfo));
        this.father.fastDrawEnd()
    }
    /**
     * set hot button
     * @param {Object} chart chart instance
     * @memberof ClChartLine
     */
    // setHotButton(chart) {
    //     for (const name in this.childCharts) {
    //         if (this.childCharts[name] === chart) {
    //             this.childCharts[name].focused = true
    //         } else {
    //             this.childCharts[name].focused = false
    //         }
    //     }
    // }
    /**
     * clear chart
     * @memberof ClChartLine
     */
    drawClear() {
        _fillRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back)
        _drawBegin(this.context, this.color.grid)
        _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)
        if (this.config.title.display !== 'none') {
            _drawHline(this.context, this.rectMain.left, this.rectMain.left + this.rectMain.width, this.rectTitle.top + this.rectTitle.height)
        }
        if (this.config.axisX.display !== 'none') {
            _drawHline(this.context, this.rectMain.left, this.rectMain.left + this.rectMain.width, this.rectAxisX.top - this.scale)
        }
        _drawEnd(this.context)
    }
    /**
     * draw child charts
     * @memberof ClChartLine
     */
    drawChildCharts() {
        let hotchart
        for (const name in this.childCharts) {
            if (!this.childCharts[name].focused) {
                this.childCharts[name].onPaint()
            } else {
                hotchart = this.childCharts[name]
            }
        }
        if (hotchart) hotchart.onPaint()
    }
    /**
     * before location
     * @memberof ClChartLine
     */
    beforeLocation() {
        for (const name in this.childLines) {
            if (this.childLines[name].beforeLocation) {
                this.childLines[name].beforeLocation()
            }
        }
    }
    /**
     * draw child lines
     * @memberof ClChartLine
     */
    drawChildLines() {
        for (const name in this.childLines) {
            if (this.childLines[name].hotKey !== undefined) {
                this.childLines[name].onPaint(this.childLines[name].hotKey)
            } else {
                this.childLines[name].onPaint(this.hotKey)
            }
        }
    }
    /**
     * According to the record index to obtain a set of data according to the keys, the data is {MA:[]...} is mainly provided for mouse movement
     * @param {Number} index
     * @return {Array}
     * @memberof ClChartLine
     */
    getMoveData(index) {
        let lines = this.config.lines
        const out = []
        if (!Array.isArray(lines)) return out

        let value, info
        for (let k = 0; k < lines.length; k++) {
            if (lines[k].info === undefined) continue
            if (lines[k].info.labelY !== undefined) {
                if (lines[k].formula === undefined) {
                    value = getValue(this.data, lines[k].info.labelY, index)
                } else {
                    value = getValue(this.father.getData(lines[k].formula.key), lines[k].info.labelY,
                        index - this.linkInfo.minIndex)
                }
                info = formatInfo(value, lines[k].info.format, this.static)
                if (lines[k].info.color === undefined) {
                    clr = getLineColor(lines[k].info.colorIndex)
                } else {
                    clr = this.color[lines[k].info.color]
                }
                out.push({
                    index: k,
                    color: clr,
                    txt: lines[k].info.txt,
                    value: info
                })
            } else {
                out.push({
                    index: k,
                    color: clr,
                    txt: lines[k].info.txt
                })
            }
        }
        return out
    }
    /**
     * draw title info
     * @param {Number} index title index
     * @memberof ClChartLine
     */
    drawTitleInfo(index) {
        if (this.config.title.display === 'none') return
        if (index === undefined || index < 0 || index > this.linkInfo.maxIndex) index = this.linkInfo.maxIndex
        this.childDraws['TITLE'].onPaint(this.getMoveData(index))
    }
    /**
     * draw child
     * @param {String} name child draw's name
     * @memberof ClChartLine
     */
    drawChildDraw(name) {
        if (this.childDraws[name] !== undefined) {
            if (name === 'TITLE') {
                this.drawTitleInfo(this.linkInfo.moveIndex)
            } else {
                this.childDraws[name].onPaint()
            }
        }
    }
    /**
     * draw all the layers contained in this area
     * @memberof ClChartLine
     */
    onPaint() {
        this.beforeLocation() // 数据定位前需要做的事情


        this.data = this.father.getData(this.hotKey)
        this.locationData()
        // console.log('onPaint', JSON.stringify(this.dataLayer.linkInfo));
        
        this.father.readyData(this.data, this.config.lines)

        _setLineWidth(this.context, this.scale)
        this.drawClear() // 清理画图区

        this.drawChildDraw('GRID') // 先画线格

        // 为画图做准备工作
        this.readyDraw()
        this.drawChildDraw('AXISX')
        this.drawChildDraw('AXISY_LEFT')
        this.drawChildDraw('AXISY_RIGHT')

        this.drawChildDraw('TITLE')
        this.drawChildLines() // 画出所有的线

        this.drawChildCharts()

        // this.img = _getImageData(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)
    }

    // 画图前的准备工作

    /**
     * get middle
     * @param {String} method
     * @return {Number}
     * @memberof ClChartLine
     */
    getMiddle(method) {
        let middle = this.config.axisY.left.middle
        if (method === 'fixedRight') middle = this.config.axisY.right.middle
        if (middle === 'before') return this.static.agoprc
        if (middle === 'zero') return 0
        return 0
    }
    /**
     * calc max and min data
     * @param {Array} data
     * @param {Object} extremum
     * @param {Number} start
     * @param {Number} stop
     * @return {Object}
     * @memberof ClChartLine
     */
    calcMaxMin(data, extremum, start, stop) {
        const mm = {
            max: 0.0,
            min: 0.0
        }
        if (extremum.method === 'fixedLeft' || extremum.method === 'fixedRight') {
            const middle = this.getMiddle(extremum.method)
            mm.min = middle * (1 - 0.01)
            mm.max = middle * (1 + 0.01)
        }
        if (data === undefined || isEmptyArray(data.value)) return mm

        let value
        if (start === undefined) start = 0
        if (stop === undefined) stop = data.value.length - 1

        for (let k = start; k <= stop; k++) {
            if (!isEmptyArray(extremum.maxvalue)) {
                for (let m = 0; m < extremum.maxvalue.length; m++) {
                    if (typeof (extremum.maxvalue[m]) !== 'string') continue
                    value = getValue(data, extremum.maxvalue[m], k)
                    if (value > mm.max) {
                        mm.max = value
                    }
                }
            }
            if (!isEmptyArray(extremum.minvalue)) {
                for (let m = 0; m < extremum.minvalue.length; m++) {
                    if (typeof (extremum.minvalue[m]) !== 'string') continue
                    value = getValue(data, extremum.minvalue[m], k)
                    if (mm.min === 0.0) mm.min = value
                    if (value < mm.min) {
                        mm.min = value
                    }
                }
            }
        }

        if (!isEmptyArray(extremum.maxvalue)) {
            for (let m = 0; m < extremum.maxvalue.length; m++) {
                if (typeof (extremum.maxvalue[m]) === 'number') {
                    mm.max = extremum.maxvalue[m]
                    break
                }
            }
        }
        if (!isEmptyArray(extremum.minvalue)) {
            for (let m = 0; m < extremum.minvalue.length; m++) {
                if (typeof (extremum.minvalue[m]) === 'number') {
                    mm.min = extremum.minvalue[m]
                    break
                }
            }
        }

        if (extremum.method === 'fixedLeft' || extremum.method === 'fixedRight') {
            const middle = this.getMiddle(extremum.method)
            if (mm.max === mm.min) {
                if (mm.max > middle) mm.min = middle
                if (mm.min < middle) mm.max = middle
            }
            const maxrate = Math.abs(mm.max - middle) / middle
            const minrate = Math.abs(middle - mm.min) / middle
            if (maxrate < 0.01 && minrate < 0.01 &&
                this.static.style !== STOCK_TYPE_IDX) {
                mm.min = middle * (1 - 0.01)
                mm.max = middle * (1 + 0.01)
            } else {
                if (maxrate > minrate) {
                    // mm.min = middle / (1 + maxrate)
                    mm.min = middle * (1 - maxrate)
                } else {
                    mm.max = middle * (1 + minrate)
                }
            }
            if (mm.min < 0) mm.min = 0
        }

        return mm
    }
    /**
     * ready for draw scroll
     * @memberof ClChartLine
     */
    readyScroll() {
        if (this.scroll.display === 'none') return
        if (this.childCharts['HSCROLL'] !== undefined) {
            this.childCharts['HSCROLL'].onChange({
                select: {
                    min: this.linkInfo.minIndex,
                    max: this.linkInfo.maxIndex
                },
                range: this.data.value.length
            })
        }
    }
    /**
     * get data range
     * @param {Array} data
     * @return {Object}
     * @memberof ClChartLine
     */
    getDataRange(data) {
        const out = {
            minIndex: -1,
            maxIndex: -1
        }
        if (isEmptyArray(data.value) || isEmptyArray(this.data.value)) return out
        const start = getValueFidx(this.data, 0, this.linkInfo.minIndex)
        const stop = getValueFidx(this.data, 0, this.linkInfo.maxIndex)
        for (let k = 0; k <= data.value.length - 1; k++) {
            const tt = getValueFidx(data, 0, k)
            if (tt >= start) {
                out.minIndex = k
                break
            }
        }
        for (let k = data.value.length - 1; k >= 0; k--) {
            const tt = getValueFidx(data, 0, k)
            if (tt <= stop) {
                out.maxIndex = k
                break
            }
        }
        return out
    }
    /**
     * location data
     * @memberof ClChartLine
     */
    locationData() {
        if (this.data === undefined) return
        const size = getSize(this.data)
        setMoveLineFlags(
            this.linkInfo, {
                width: this.rectChart.width,
                scale: this.scale,
                size
            }
        )
        // console.log('link', JSON.stringify(this.linkInfo));
    }
    /**
     * set ready for draw
     * @memberof ClChartLine
     */
    readyDraw() { // 计算最大最小值等
        // 画滚动块
        this.readyScroll()

        // 求最大最小值
        let mm, maxmin
        // const force = true;   // 由于定义了没有在后面使用，因此注释掉
        for (let i = 0; i < this.config.lines.length; i++) {
            if (this.config.lines[i].extremum === undefined) continue
            if (isEmptyArray(this.config.lines[i].extremum.maxvalue) &&
                isEmptyArray(this.config.lines[i].extremum.minvalue)) continue
            // 只对第一个线和有需要的线计算最大最小值
            // const ds = this.getLineDS(this.config.lines[i]);
            const formula = this.config.lines[i].formula
            if (formula !== undefined) {
                const newdata = this.father.getData(formula.key)
                const range = this.getDataRange(newdata)
                mm = this.calcMaxMin(newdata, this.config.lines[i].extremum,
                    range.minIndex, range.maxIndex)
            } else {
                mm = this.calcMaxMin(this.data, this.config.lines[i].extremum,
                    this.linkInfo.minIndex, this.linkInfo.maxIndex)
            }
            if (maxmin === undefined) maxmin = mm
            else {
                maxmin.max = maxmin.max > mm.max ? maxmin.max : mm.max
                maxmin.min = maxmin.min < mm.min ? maxmin.min : mm.min
            }
        } // for
        this.axisYdata.max = maxmin.max
        this.axisYdata.min = maxmin.min
        this.axisYdata.unitY = (this.rectChart.height - 2) / (this.axisYdata.max - this.axisYdata.min) // 一个单位价位多少像素
    }

    /**
     * handle click event
     * @param {Object} event
     * @memberof ClChartLine
     */
    onClick(event) {
        if (this.axisPlatform !== 'phone') {
            this.linkInfo.showCursorLine = !this.linkInfo.showCursorLine
            if (this.linkInfo.showCursorLine) {
                this.father.eventLayer.boardEvent(this.father, 'onMouseMove', event)
            } else {
                event.reDraw = true // 需要重画
                this.father.eventLayer.boardEvent(this.father, 'onMouseOut', event)
            }
        }
    }
    /**
     * handle long press
     * @param {Object} event
     * @memberof ClChartLine
     */
    onLongPress(event) {
        this.linkInfo.showCursorLine = true
        this.father.eventLayer.boardEvent(this.father, 'onMouseMove', event)
    }
    /**
     * handle pinch
     * @param {Object} event
     * @memberof ClChartLine
     */
    // onPinch(event) {
    //     if (!this.config.zoomInfo) {
    //         return
    //     }
    //     if (event.scale > 0) {
    //         this.linkInfo.zoomIndex++
    //     } else {
    //         this.linkInfo.zoomIndex--
    //     }
    //     if (this.linkInfo.zoomIndex < 0) {
    //         this.linkInfo.zoomIndex = 0
    //         return
    //     }
    //     // this.setZoomInfo(Math.pow(2, this.linkInfo.zoomIndex) + 1)
    //     this.setZoomInfo(this.linkInfo.zoomIndex * 2 + 1)
    //     this.father.onPaint()
    // }
    /**
     * handle mouse out
     * @param {Object} event
     * @memberof ClChartLine
     */
    onMouseOut(event) {
        if (this.linkInfo.showCursorLine || event.reDraw) {
            this.linkInfo.moveIndex = this.linkInfo.maxIndex
            this.drawTitleInfo(this.linkInfo.moveIndex)
            // this.father.onPaint(this)
        }
        this.childDraws['CURSOR'].onClear()
    }
    /**
     * handle mouse wheel
     * @param {Object} event
     * @memberof ClChartLine
     */
    onMouseWheel(event) {
        if (this.config.zoomInfo === undefined) return

        let step = Math.floor(event.deltaY / 10)
        if (step === 0) step = event.deltaY > 0 ? 1 : -1
        let newIndex
        if (step > 0) newIndex = this.linkInfo.zoomIndex - 1
        else newIndex = this.linkInfo.zoomIndex + 1
        if (newIndex >= 0 && newIndex <= this.config.zoomInfo.list.length - 1) {
            this.linkInfo.zoomIndex = newIndex
            this.setZoomInfo()
            this.father.onPaint()
        }
    }
    /**
     * handle key down
     * @param {Object} event
     * @memberof ClChartLine
     */
    onKeyDown(event) {
        switch (event.keyCode) {
            case 38: // up
                if (this.config.zoomInfo && this.linkInfo.zoomIndex < this.config.zoomInfo.list.length - 1) {
                    this.linkInfo.zoomIndex++
                    this.setZoomInfo()
                    this.father.onPaint()
                }
                break
            case 40: // down
                if (this.config.zoomInfo && this.linkInfo.zoomIndex > 0) {
                    this.linkInfo.zoomIndex--
                    this.setZoomInfo()
                    this.father.onPaint()
                }
                break
            case 37: // left
                break
            case 39: // right
                break
            case 32: //
                this.callback({
                    event: 'keydown',
                    key: ' ',
                    code: 'Space',
                    keyCode: 32,
                    axisXinfo: this.father.dataLayer.axisXinfo.dataTxt[this.linkInfo.moveIndex]
                })
                break
        }
    }
    /**
     * handle mouse move
     * @param {Object} event
     * @memberof ClChartLine
     */
    onMouseMove(event) {
        if (!this.linkInfo.showCursorInfo) return
        if (!this.linkInfo.showCursorLine) return
        // console.log(JSON.stringify(this.dataLayer.linkInfo));

        if (event.board === undefined || event.board === 0) {
            this.father.eventLayer.boardEvent(this.father, 'onMouseMove', event, this)
        }
        // this.draw_clear();
        // 找到X坐标对应的数据索引
        const mousePos = event.mousePos
        // if (this.img !== undefined) {
        //   _putImageData(this.context, this.img, this.rectMain.left, this.rectMain.top)
        // }
        const mouseIndex = this.getMouseMoveData(mousePos.x)
        // console.log(this.linkInfo.minIndex, mouseIndex, this.rectChart, inRangeX(this.rectChart, mousePos.x))
        let valueY
        let valueX = mouseIndex
        if (mouseIndex > 0) {
            const dataTxt = this.father.dataLayer.axisXinfo.dataTxt
            if (mouseIndex < dataTxt.length) {
                valueX = dataTxt[mouseIndex]
            } else {
                valueX = dataTxt.at(-1)
            }
            if (inRangeX(this.rectChart, mousePos.x)) {
                this.drawTitleInfo(mouseIndex)
            }
            if (this.linkInfo.moveIndex !== mouseIndex) {
                this.linkInfo.moveIndex = mouseIndex
                this.callback({
                    event: 'mousemove',
                    static: this.static,
                    data: this.data.value[mouseIndex]
                })
            }
        }

        this.childDraws['CURSOR'].onPaint(mousePos, valueX, valueY)
    }
    /**
     * get mouse move data
     * @param {Number} xpos
     * @return {Number}
     * @memberof ClChartLine
     */
    getMouseMoveData(xpos) {
        let index = Math.round((xpos - this.rectChart.left) / (this.linkInfo.unitX + this.linkInfo.spaceX))
        index = Math.max(0, index)
        return this.linkInfo.minIndex + index
    }

    // /// ClChart end.
}
