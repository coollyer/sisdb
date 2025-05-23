/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 可以拖动的滑动块定义
// 支持锁定模式和自由模式，锁定模式滑动块固定大小，和传统滑动块类似，自由模式可左右选择范围，并返回值域，
// 通常定义最大最小值，当前选定的最大最小值，并在鼠标放开后返回当前值域

import {
  _getTxtWidth,
  _setLineWidth,
  _drawTxt,
  _fillRect,
  _drawRect,
  _drawBegin,
  _drawEnd
} from './cl.draw'
import {
  changeCursorStyle,
  initCommonInfo,
  checkLayout
} from './cl.chart.init'
import { CHART_LAYOUT } from '../cl.chart.def'
import {
  updateJsonOfDeep,
  offsetRect,
  inRect
} from '../cl.utils'

const DEFAULT_SCROLL = {
  // display: 'none', // true
  shape: 'fixed', // fixed 为固定宽度 free有边界
  direct: 'horizontal', // ver 竖立 和横
  range: 100,
  select: { min: 40, max: 60 }, // min == beginIdx max = pageCount
  status: 'enabled',
  txt: {}
}
/**
 * Class representing ClChartScroll
 * @export
 * @class ClChartScroll
 */
export default class ClChartScroll {
  /**

   * Creates an instance of ClChartScroll.
   * @param {Object} father order chart's parent context
   */
  constructor (father) {
    initCommonInfo(this, father)
  }
  /**
   * init
   * @param {Object} cfg
   * @param {any} callback initialized callback
   * @memberof ClChartScroll
   */
  init (cfg, callback) {
    this.callback = callback
    this.rectMain = cfg.rectMain || { left: 0, top: 0, width: 200, height: 25 }
    this.layout = updateJsonOfDeep(cfg.layout, CHART_LAYOUT)
    this.config = updateJsonOfDeep(cfg.config, DEFAULT_SCROLL)

    // 下面对配置做一定的校验
    this.checkConfig()
    // 再做一些初始化运算，下面的运算范围是初始化设置后基本不变的数据
    this.setPublicRect()
  }
  /**
   * check config
   * @memberof ClChartScroll
   */
  checkConfig () {
    checkLayout(this.layout)
  }
  /**
   * Calculate all rectangular areas
   * @memberof ClChartScroll
   */
  setPublicRect () { // 计算所有矩形区域
    const count = this.config.range
    let spaceX

    this.rectChart = offsetRect(this.rectMain, this.layout.margin)

    let start, stop, spaceY
    if (this.config.direct === 'horizontal') {
      spaceX = this.rectMain.width / (count - 1)
      spaceY = this.rectMain.height - this.scale * 2
      start = this.rectMain.left + spaceX * this.config.select.min
      stop = this.rectMain.left + spaceX * this.config.select.max
      if (this.config.shape === 'free') {
        this.rectMin = {
          left: start - this.layout.scroll.size / 2,
          top: this.rectMain.top + this.scale,
          width: this.layout.scroll.size,
          height: spaceY
        }
        this.rectMax = {
          left: stop - this.layout.scroll.size / 2,
          top: this.rectMain.top + this.scale,
          width: this.layout.scroll.size,
          height: spaceY
        }
        this.rectMid = {
          left: start + this.layout.scroll.size / 2,
          top: this.rectMain.top + this.scale,
          width: stop - start - this.layout.scroll.size,
          height: spaceY
        }
      } else {
        this.rectMid = {
          left: start,
          top: this.rectMain.top + this.scale,
          width: stop - start,
          height: spaceY
        }
      }
    } else {
      // spaceX = this.rectMain.height / (count - 1);
      // spaceY = this.rectMain.width - this.scale * 2;
      // start = this.rectMain.top + spaceX * this.config.select;
      // stop = this.rectMain.top + spaceX * this.config.select;
    }
  }

  /**
   * paint scroll chart
   * @memberof ClChartScroll
   */
  onPaint () {
    _setLineWidth(this.context, this.scale)

    this.drawClear()
    this.drawGridline() // 先画线格
    this.setPublicRect()
    this.drawButton()
  }
  /**
   * handle change info
   * @param {Object} info
   * @memberof ClChartScroll
   */
  onChange (info) {
    this.config = updateJsonOfDeep(info, this.config)
    if (this.config.select.max > this.config.range) this.config.select.max = this.config.range - 1

    if (info.iscall) {
      this.callback({ self: this, minIndex: this.config.select.min })
    }
  }
  /**
   * find mouse index
   * @param {Number} xpos
   * @return {Number} index
   * @memberof ClChartScroll
   */
  findMouseIndex (xpos) {
    const count = this.config.range
    const spaceX = this.rectMain.width / (count - 1)
    const idx = Math.round((xpos - this.rectMain.left) / spaceX)
    return idx
  }
  /**
   * check min
   * @param {Number} min
   * @return {Number}
   * @memberof ClChartScroll
   */
  checkMin (min) {
    if (min < 0) return 0
    if (min > this.config.range - (this.config.select.max - this.config.select.min + 1)) {
      return this.config.range - (this.config.select.max - this.config.select.min + 1)
    }
    return min
  }
  /**
   * on init
   * @memberof ClChartScroll
   */
  onInit () {
    changeCursorStyle('default')
    this.who = undefined
  }
  /**
   * on click
   * @memberof ClChartScroll
   */
  onClick (event) {
    // 阻止click再传递
    event.break = true
  }
  /**
   * handle mouse out
   * @memberof ClChartScroll
   */
  onMouseOut (event) {
    if (this.mouseDown) this.onMouseUp(event)
    changeCursorStyle('default')
    this.who = undefined
  }
  /**
   * hanlde mouse down
   * @param {Object} event
   * @memberof ClChartScroll
   */
  onMouseDown (event) {
    const mousePos = event.mousePos
    if (inRect(this.rectMin, mousePos)) {
      this.who = 'min'
    } else if (inRect(this.rectMax, mousePos)) {
      this.who = 'max'
    } else if (inRect(this.rectMid, mousePos)) {
      this.who = 'mid'
      this.index = this.findMouseIndex(mousePos.x)
    } else {
      this.who = undefined
    }
    this.mouseDown = true
  }
  /**
   * handle mouse up
   * @param {Object} event
   * @memberof ClChartScroll
   */
  onMouseUp (event) {
    this.mouseDown = false
    if (this.config.shape !== 'free' && this.who === undefined) {
    // if (this.config.shape !== 'free') {
      const mousePos = event.mousePos
      const curIndex = this.findMouseIndex(mousePos.x)
      let min = curIndex - Math.floor((this.config.select.max - this.config.select.min) / 2)
      min = this.checkMin(min)
      this.onChange({ select: { min }, iscall: true })
    }
    this.who = undefined
  }
  /**
   * handle mouse move
   * @param {Object} event
   * @memberof ClChartScroll
   */
  onMouseMove (event) {
    if (!this.mouseDown) return
    const mousePos = event.mousePos
    if (inRect(this.rectMin, mousePos) || inRect(this.rectMax, mousePos)) {
      changeCursorStyle('col-resize')
    } else if (inRect(this.rectMid, mousePos)) {
      changeCursorStyle('move')
    } else {
      changeCursorStyle('default')
    }
    if (this.who !== undefined) {
      let min, max
      const curIndex = this.findMouseIndex(mousePos.x)
      if (this.config.shape === 'free') {
        if (curIndex < this.config.select.max) {
          if (this.who === 'max') {
            max = curIndex
          } else if (this.who === 'min') {
            min = curIndex
          }
        } else if (curIndex >= this.config.select.max) {
          if (this.who === 'min') {
            min = this.config.select.max
            max = curIndex
            this.who = 'max'
          } else if (this.who === 'max') {
            max = curIndex
          }
        }
        this.onChange({ select: { min, max }, iscall: true })
      } else {
        min = this.config.select.min + curIndex - this.index
        this.index = curIndex
        min = this.checkMin(min)
        this.onChange({ select: { min }, iscall: true })
      }
    }
  }
  /**
   * clear chart
   * @memberof ClChartScroll
   */
  drawClear () {
    _fillRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back)
  }
  /**
   * draw grid line
   * @memberof ClChartScroll
   */
  drawGridline () {
    _drawBegin(this.context, this.color.grid)
    _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width + this.scale / 2, this.rectMain.height)
    _drawEnd(this.context)
  }
  drawButton () {
    if (this.config.direct === 'horizontal') {
      const spaceY = (this.rectChart.height - this.layout.scroll.height) / 2
      if (this.config.txt.head !== undefined) {
        _drawTxt(this.context, this.rectChart.left + this.scale, this.rectChart.top + spaceY,
          this.config.txt.head,
          this.layout.scroll.font, this.layout.scroll.pixel, this.color.axis)
      }
      if (this.config.txt.tail !== undefined) {
        _drawTxt(this.context, this.rectChart.left + this.rectChart.width - this.scale,
          this.rectChart.top + spaceY, this.config.txt.tail,
          this.layout.scroll.font, this.layout.scroll.pixel, this.color.axis, { x: 'end' })
      }

      _drawBegin(this.context, this.color.frame)
      _drawRect(this.context, this.rectMid.left, this.rectMid.top,
        this.rectMid.width, this.rectMid.height)
      _fillRect(this.context, this.rectMid.left, this.rectMid.top,
        this.rectMid.width, this.rectMid.height, this.color.box)

      if (this.config.shape === 'free') {
        _fillRect(this.context, this.rectMin.left, this.rectMin.top,
          this.rectMin.width, this.rectMin.height, this.color.frame)
        _fillRect(this.context, this.rectMax.left, this.rectMax.top,
          this.rectMax.width, this.rectMax.height, this.color.frame)
      }
      _drawEnd(this.context)
      const len = _getTxtWidth(this.context, this.config.txt.left, this.layout.scroll.font, this.layout.scroll.pixel) + 2 * this.scale
      if (this.config.txt.left !== undefined && this.rectMid.width > len) {
        _drawTxt(this.context, this.rectMid.left + this.scale, this.rectMid.top + spaceY,
          this.config.txt.left,
          this.layout.scroll.font, this.layout.scroll.pixel, this.color.axis)
      }
      if (this.config.txt.right !== undefined && this.rectMid.width > 2 * len) {
        _drawTxt(this.context, this.rectMid.left + this.rectMid.width - this.scale,
          this.rectMid.top + spaceY, this.config.txt.right,
          this.layout.scroll.font, this.layout.scroll.pixel, this.color.axis, { x: 'end' })
      }
    }
  }
}
