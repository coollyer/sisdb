/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClLineAxisY 的实体定义

import {
  _drawTxt
} from './cl.draw'
import {
  initCommonInfo
} from './cl.chart.init'
import {
  formatInfo
} from '../cl.utils'

// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawAxisY
 * @export
 * @class ClDrawAxisY
 */
export default class ClDrawAxisY {
  /**

   * Creates an instance of ClDrawAxisY.
   * @param {Object} father
   * @param {Object} rectMain
   * @param {Object} align
   */
  constructor (father, rectMain, align) {
    initCommonInfo(this, father)
    this.rectMain = rectMain

    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo
    this.static = this.dataLayer.keyInfo[this.linkInfo.curKey].static
    
    this.axisYdata = father.axisYdata
    this.align = align
    this.axisY = father.config.axisY

    this.text = father.layout.title
  }
  /**
   * paint
   * @memberof ClDrawAxisY
   */
  onPaint () {
    if (this.axisY[this.align].display === 'none') return
    if (!this.linkInfo.showCursorInfo) return

    let xx, yy
    let value, clr

    let posX
    const offX = this.axisPlatform === 'phone' ? 2 * this.scale : -2 * this.scale

    if (this.align === 'left') {
      if (this.axisPlatform === 'phone') {
        posX = 'start'
        xx = this.rectMain.left + offX
      } else {
        posX = 'end'
        xx = this.rectMain.left + this.rectMain.width + offX
      }
    } else {
      if (this.axisPlatform === 'phone') {
        posX = 'end'
        xx = this.rectMain.left + this.rectMain.width - offX
      } else {
        posX = 'start'
        xx = this.rectMain.left - offX
      }
    }
    yy = this.rectMain.top + this.scale // 画最上面的

    // 画不画最上面的坐标
    if (this.axisY[this.align].display !== 'noupper') {
      yy = this.rectMain.top + this.scale // 画最上面的
      clr = this.axisY[this.align].middle !== 'before' ? this.color.axis : this.color.red
      value = formatInfo(
        this.axisYdata.max,
        this.axisY[this.align].format,
        this.static)
      _drawTxt(this.context, xx, yy, value,
        this.text.font, this.text.pixel, clr,
        { x: posX, y: 'top' })
    }
    // 画不画最下面的坐标
    if (this.axisY[this.align].display !== 'nofoot') {
      clr = this.axisY[this.align].middle !== 'before' ? this.color.axis : this.color.green
      yy = this.rectMain.top + this.rectMain.height - this.scale
      value = formatInfo(
        this.axisYdata.min,
        this.axisY[this.align].format,
        this.static)
      _drawTxt(this.context, xx, yy, value,
        this.text.font, this.text.pixel, clr,
        { x: posX, y: 'bottom' })
    }
    // 画其他的坐标线
    const offy = this.rectMain.height / (this.axisY.lines + 1)

    for (let i = 0; i < this.axisY.lines; i++) {
      value = this.axisYdata.max - offy * (i + 1) / this.axisYdata.unitY
      yy = this.rectMain.top + Math.round((i + 1) * offy)
      clr = this.color.axis
      if (this.axisY[this.align].middle !== 'none') {
        if ((i + 1) < Math.round(this.axisY.lines / 2)) clr = this.color.red
        if ((i + 1) > Math.round(this.axisY.lines / 2)) clr = this.color.green
        if ((i + 1) === Math.round(this.axisY.lines / 2)) {
          value = this.axisY[this.align].middle === 'zero' ? 0 : this.static.agoprc
        }
      }

      value = formatInfo(
        value,
        this.axisY[this.align].format,
        this.static)

      _drawTxt(this.context, xx, yy, value,
        this.text.font, this.text.pixel, clr,
        { x: posX, y: 'middle' })
    }
  }
}
