/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

//   画买卖盘

// 画买卖盘和Tick

import {
  _fillRect,
  _drawRect,
  _drawHline,
  _setLineWidth,
  _drawTxt,
  _getTxtWidth,
  _drawBegin,
  _drawEnd
} from './cl.draw'
import {
  fromTTimeToStr,
  formatVolume,
  formatPrice,
  updateJsonOfDeep,
  offsetRect
} from '../cl.utils'
import {
  initCommonInfo,
  checkLayout
} from './cl.chart.init'
import {
  CHART_LAYOUT,
  CHART_BOARD
} from '../cl.chart.def'
import getValue from '../datas/cl.data.tools'
import {
    STOCK_TYPE_IDX,
    FIELD_SNAP,
    FIELD_SNAP_IDX,
    FIELD_TICK
} from '../cl.data.def'

/**
 * Class representing ClChartBoard
 * @export
 * @class ClChartBoard
 */
export default class ClChartBoard {
  /**

   * Creates an instance of ClChartBoard.
   * @param {Object} father order chart's parent context
   */
  constructor (father) {
    initCommonInfo(this, father)
    this.linkInfo = father.linkInfo
    this.static = this.father.dataLayer.static
  }
  /**
   * init order chart
   * @param {Object} cfg
   * @memberof ClChartBoard
   */
  init (cfg) {
    this.rectMain = cfg.rectMain || { left: 0, top: 0, width: 200, height: 300 }
    this.layout = updateJsonOfDeep(cfg.layout, CHART_LAYOUT)

    this.config = updateJsonOfDeep(cfg.config, CHART_BOARD)

    this.style = cfg.config.style || 'normal'
    // 下面对配置做一定的校验
    this.checkConfig()
    // 再做一些初始化运算，下面的运算范围是初始化设置后基本不变的数据
    this.setPublicRect()
  }
  /**
   * check config
   * @memberof ClChartBoard
   */
  checkConfig () { // 检查配置有冲突的修正过来
    checkLayout(this.layout)
    this.txtLen = _getTxtWidth(this.context, '涨', this.layout.digit.font, this.layout.digit.pixel)
    this.timeLen = _getTxtWidth(this.context, '15:30', this.layout.digit.font, this.layout.digit.pixel)
    this.volLen = _getTxtWidth(this.context, '888888', this.layout.digit.font, this.layout.digit.pixel)
    this.closeLen = _getTxtWidth(this.context, '888.88', this.layout.digit.font, this.layout.digit.pixel)
  }
  /**
   * Calculate all rectangular areas
   * @memberof ClChartBoard
   */
  setPublicRect () {
    this.rectChart = offsetRect(this.rectMain, this.layout.margin)
  }
  /**
   * handle click event
   * @memberof ClChartBoard
   */
  onClick (/* e */) {
    if (this.isIndex) return // 如果是指数就啥也不干
    if (this.style === 'normal') {
      this.style = 'tiny'
    } else {
      this.style = 'normal'
    }
    this.father.onPaint(this)
  }
  /**
   * paint order chart
   * @memberof ClChartBoard
   */
  onPaint () {
    this.codeInfo = this.father.getData('STKINFO')
    this.snapData =  this.father.getData('SNAPS')
    // 根据 snapData 生成 需要的数据
    this.orderData = this.father.getData('ORDER')
    this.tickData = this.father.getData('TICK')
    if (this.orderData === undefined || this.tickData === undefined) return
    this.orderData.coindot = this.static.coindot
    this.tickData.coindot = this.static.coindot
    this.isIndex = getValue(this.codeInfo, 'style') === STOCK_TYPE_IDX

    _setLineWidth(this.context, this.scale)
    this.drawClear() // 清理画图区
    this.drawReady() // 数据准备

    if (this.isIndex) {
      this.drawIndex()
    } else {
      this.drawOrder()
    }
    this.drawTick()
  }
  /**
   * clear chart
   * @memberof ClChartBoard
   */
  drawClear () {
    _fillRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back)
  }
  /**
   * set ready for draw
   * @memberof ClChartBoard
   */
  drawReady () {
    if (this.tickData === undefined) {
      this.tickData = {
        key: 'TICK',
        fields: FIELD_TICK,
        value: []
      }
    }
    if (this.orderData === undefined) {
      this.orderData = {
        key: 'NOW',
        fields: FIELD_SNAP,
        value: []
      }
    }
    let yy
    if (this.style === 'normal') {
      yy = this.rectChart.top + (this.layout.digit.height + this.layout.digit.spaceX) * 10
    } else {
      yy = this.rectChart.top + (this.layout.digit.height + this.layout.digit.spaceX) * 2
    }
    if (this.isIndex) {
      yy = this.rectChart.top + (this.layout.digit.height + this.layout.digit.spaceX) * 4
      this.rectOrder = {
        left: this.rectChart.left,
        top: this.rectChart.top,
        width: this.rectChart.width,
        height: yy
      }
    } else {
      this.rectOrder = {
        left: this.rectChart.left,
        top: this.rectChart.top,
        width: this.rectChart.width,
        height: yy
      }
    }
    if (this.config.title.display !== 'none') {
      this.rectTitle = {
        left: this.rectChart.left,
        top: yy,
        width: this.rectChart.width,
        height: this.layout.title.height
      }
    } else {
      this.rectTitle = {
        left: 0,
        top: 0,
        width: 0,
        height: 0
      }
    }
    yy += this.rectTitle.height
    this.rectTick = {
      left: this.rectChart.left,
      top: yy,
      width: this.rectChart.width,
      height: this.rectChart.height - yy - this.layout.digit.height / 2
    }
  }
  /**
   * get color by close and before data
   * @param {Number} close
   * @param {Number} before
   * @return {String} color
   * @memberof ClChartBoard
   */
  getColor (close, before) {
    if (close > before) {
      return this.color.red
    } else if (close < before) {
      return this.color.green
    } else {
      return this.color.white
    }
  }
  /**
   * draw index
   * @memberof ClChartBoard
   */
  drawIndex () {
    _drawBegin(this.context, this.color.grid)
    _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)

    const offy = this.rectOrder.height / 3
    const offx = this.rectOrder.width / 3
    let xx, yy, len
    let value

    yy = this.rectOrder.top + Math.floor((offy - this.layout.digit.height) / 2) // 画最上面的

    xx = this.rectOrder.left + (offx - this.txtLen) / 2
    _drawTxt(this.context, xx, yy, '涨', this.layout.digit.font, this.layout.digit.pixel, this.color.txt)
    xx = this.rectOrder.left + offx + (offx - this.txtLen) / 2
    _drawTxt(this.context, xx, yy, '跌', this.layout.digit.font, this.layout.digit.pixel, this.color.txt)
    xx = this.rectOrder.left + 2 * offx + (offx - this.txtLen) / 2
    _drawTxt(this.context, xx, yy, '平', this.layout.digit.font, this.layout.digit.pixel, this.color.txt)

    const inow = {
      key: 'NOW',
      fields: FIELD_SNAP_IDX,
      value: this.orderData.value
    }
    yy = this.rectOrder.top + offy + Math.floor((offy - this.layout.digit.height) / 2) // 画最上面的
    value = formatVolume(getValue(inow, 'ups'), 1)
    len = _getTxtWidth(this.context, value, this.layout.digit.font, this.layout.digit.pixel)
    xx = this.rectOrder.left + (offx - len) / 2
    _drawTxt(this.context, xx, yy, value, this.layout.digit.font, this.layout.digit.pixel, this.color.txt)

    value = formatVolume(getValue(inow, 'downs'), 1)
    len = _getTxtWidth(this.context, value, this.layout.digit.font, this.layout.digit.pixel)
    xx = this.rectOrder.left + offx + (offx - len) / 2
    _drawTxt(this.context, xx, yy, value, this.layout.digit.font, this.layout.digit.pixel, this.color.txt)

    value = formatVolume(getValue(inow, 'mids'), 1)
    len = _getTxtWidth(this.context, value, this.layout.digit.font, this.layout.digit.pixel)
    xx = this.rectOrder.left + 2 * offx + (offx - len) / 2
    _drawTxt(this.context, xx, yy, value, this.layout.digit.font, this.layout.digit.pixel, this.color.txt)

    yy = this.rectOrder.top + 2 * offy + Math.floor((offy - this.layout.digit.height) / 2)

    value = formatVolume(getValue(inow, 'upvol'), 1)
    len = _getTxtWidth(this.context, value, this.layout.digit.font, this.layout.digit.pixel)
    xx = this.rectOrder.left + (offx - len) / 2
    _drawTxt(this.context, xx, yy, value, this.layout.digit.font, this.layout.digit.pixel, this.color.txt)

    value = formatVolume(getValue(inow, 'downvol'), 1)
    len = _getTxtWidth(this.context, value, this.layout.digit.font, this.layout.digit.pixel)
    xx = this.rectOrder.left + offx + (offx - len) / 2
    _drawTxt(this.context, xx, yy, value, this.layout.digit.font, this.layout.digit.pixel, this.color.txt)

    if (this.config.title.display !== 'none') {
      _drawHline(this.context, this.rectTitle.left, this.rectTitle.left + this.rectTitle.width, this.rectTitle.top)
      _drawHline(this.context, this.rectTitle.left, this.rectTitle.left + this.rectTitle.width, this.rectTitle.top + this.rectTitle.height)
      const ticklen = _getTxtWidth(this.context, '分时成交', this.layout.title.font, this.layout.digit.pixel)
      xx = this.rectTitle.left + (this.rectTitle.width - ticklen) / 2
      yy = this.rectTitle.top + 3 * this.scale
      _drawTxt(this.context, xx, yy, '分时成交',
        this.layout.digit.font, this.layout.digit.pixel, this.color.txt)
    }
    _drawEnd(this.context)
  }
  /**
   * draw order
   * @memberof ClChartBoard
   */
  drawOrder () {
    const xpos = this.drawGridLine() // 先画线格
    if (this.orderData === undefined || this.orderData.value.length < 1) {
      return
    }
    const offx = (this.rectOrder.width - xpos - 2 * this.layout.digit.spaceX - this.closeLen - this.volLen) / 2

    let mmpCount = 1
    if (this.style === 'normal') {
      mmpCount = 5
    }
    const offy = this.rectOrder.height / (mmpCount * 2)

    let xx, yy
    let value, clr

    yy = this.rectOrder.top + Math.floor((offy - this.layout.digit.height) / 2) // 画最上面的
    for (let idx = mmpCount; idx >= 1; idx--) {
      xx = this.rectOrder.left + xpos + offx + this.closeLen
      if (this.linkInfo.showCursorInfo) {
        value = getValue(this.orderData, 'bidp' + idx)
        clr = this.getColor(value, this.static.before)
        _drawTxt(this.context, xx, yy, formatPrice(value, this.static.coindot),
          this.layout.digit.font, this.layout.digit.pixel, clr, {
            x: 'end'
          })
      }

      xx += offx + this.volLen + this.layout.digit.spaceX
      value = getValue(this.orderData, 'bidv' + idx)
      clr = this.color.vol
      _drawTxt(this.context, xx, yy, formatVolume(value, this.static.volzoom),
        this.layout.digit.font, this.layout.digit.pixel, clr, {
          x: 'end'
        })

      yy += offy
    }
    for (let idx = 1; idx <= mmpCount; idx++) {
      xx = this.rectOrder.left + xpos + offx + this.closeLen
      if (this.linkInfo.showCursorInfo) {
        value = getValue(this.orderData, 'askp' + idx)
        clr = this.getColor(value, this.static.before)
        _drawTxt(this.context, xx, yy, formatPrice(value, this.static.coindot),
          this.layout.digit.font, this.layout.digit.pixel, clr, {
            x: 'end'
          })
      }
      xx += offx + this.volLen + this.layout.digit.spaceX
      value = getValue(this.orderData, 'askv' + idx)
      clr = this.color.vol
      _drawTxt(this.context, xx, yy, formatVolume(value, this.static.volzoom),
        this.layout.digit.font, this.layout.digit.pixel, clr, {
          x: 'end'
        })

      yy += offy
    }
  }
  /**
   * draw tick
   * @memberof ClChartBoard
   */
  drawTick () {
    if (this.tickData === undefined || this.tickData.value.length < 1) return
    const maxlines = Math.floor(this.rectTick.height / this.layout.digit.height) - 1 // 屏幕最大能显示多少条记录
    const recs = this.tickData.value.length
    const beginIndex = recs > maxlines ? recs - maxlines : 0
    const offy = this.rectTick.height / maxlines

    let xx, yy
    let value, clr
    let offx = (this.rectTick.width - 4 * this.layout.digit.spaceX - this.timeLen - this.closeLen - this.volLen) / 2
    if (this.isIndex) offx = (this.rectTick.width - 3 * this.layout.digit.spaceX - this.timeLen - this.closeLen) / 2

    yy = this.rectTick.top + 2 + Math.floor((offy - this.layout.digit.pixel) / 2) // 画最上面的
    for (let idx = recs - 1; idx >= beginIndex; idx--) {
      // for (let idx = beginIndex; idx < recs; idx++) {
      xx = this.rectTick.left + this.layout.digit.spaceX + this.timeLen
      value = getValue(this.tickData, 'time', idx)
      clr = this.color.txt
      let str
      if (idx === 0) {
        str = fromTTimeToStr(value, 'minute')
      } else {
        str = fromTTimeToStr(value, 'minute', getValue(this.tickData, 'time', idx - 1))
      }
      _drawTxt(this.context, xx, yy, str,
        this.layout.digit.font, this.layout.digit.pixel, clr, {
          x: 'end'
        })

      if (this.isIndex) {
        xx = this.rectTick.left + this.rectTick.width - this.layout.digit.spaceX

        value = getValue(this.tickData, 'close', idx)
        clr = this.getColor(value, this.static.before)
        _drawTxt(this.context, xx, yy, formatPrice(value, this.static.coindot),
          this.layout.digit.font, this.layout.digit.pixel, clr, {
            x: 'end'
          })
        yy += offy
        continue
      }
      xx += offx + this.closeLen + this.layout.digit.spaceX

      if (this.linkInfo.showCursorInfo) {
        value = getValue(this.tickData, 'close', idx)
        clr = this.getColor(value, this.static.before)
        _drawTxt(this.context, xx, yy, formatPrice(value, this.static.coindot),
          this.layout.digit.font, this.layout.digit.pixel, clr, {
            x: 'end'
          })
      }
      xx += offx + this.volLen + this.layout.digit.spaceX

      value = getValue(this.tickData, 'decvol', idx)
      clr = this.color.vol
      _drawTxt(this.context, xx, yy, formatVolume(value, this.static.volzoom),
        this.layout.digit.font, this.layout.digit.pixel, clr, {
          x: 'end'
        })

      yy += offy
    }
  }
  /**
   * draw grid line
   * @return {Number}
   * @memberof ClChartBoard
   */
  drawGridLine () {
    _drawBegin(this.context, this.color.grid)
    _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)

    let mmpCount = 1
    if (this.style === 'normal') {
      mmpCount = 5
    }
    let len = 0
    _drawHline(this.context, this.rectOrder.left, this.rectOrder.left + this.rectOrder.width, this.rectOrder.top + Math.floor(this.rectOrder.height / 2))

    let xx, yy, value
    const strint = ['①', '②', '③', '④', '⑤']
    const offy = this.rectOrder.height / (mmpCount * 2)

    len = _getTxtWidth(this.context, '卖①', this.layout.title.font, this.layout.digit.height)
    yy = this.rectOrder.top + Math.floor((offy - this.layout.digit.pixel) / 2) // 画最上面的
    for (let idx = mmpCount - 1; idx >= 0; idx--) {
      xx = this.rectOrder.left + this.layout.digit.spaceX
      value = '卖' + strint[idx]
      _drawTxt(this.context, xx, yy, value,
        this.layout.digit.font, this.layout.digit.pixel, this.color.txt)
      yy += offy
    }
    for (let idx = 0; idx < mmpCount; idx++) {
      xx = this.rectOrder.left + this.layout.digit.spaceX
      value = '买' + strint[idx]
      _drawTxt(this.context, xx, yy, value,
        this.layout.digit.font, this.layout.digit.pixel, this.color.txt)
      yy += offy
    }
    if (this.config.title.display !== 'none') {
      _drawHline(this.context, this.rectTitle.left, this.rectTitle.left + this.rectTitle.width, this.rectTitle.top)
      _drawHline(this.context, this.rectTitle.left, this.rectTitle.left + this.rectTitle.width, this.rectTitle.top + this.rectTitle.height)
      if (this.style === 'normal') {
        value = '分时成交 △'
      } else {
        value = '分时成交 ▽'
      }
      const ticklen = _getTxtWidth(this.context, value, this.layout.title.font, this.layout.digit.pixel)
      xx = this.rectTitle.left + (this.rectTitle.width - ticklen) / 2
      yy = this.rectTitle.top + 3 * this.scale
      _drawTxt(this.context, xx, yy, value,
        this.layout.digit.font, this.layout.digit.pixel, this.color.txt)
    }
    _drawEnd(this.context)
    return len
  }
}
