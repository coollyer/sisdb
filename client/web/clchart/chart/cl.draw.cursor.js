/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClDrawCursor 的实体定义

import {
  _getTxtWidth,
  _drawTxtRect,
  _clearRect,
  _drawLineAlone
} from './cl.draw'
import {
  initCommonInfo
} from './cl.chart.init'
import {
  inRangeX,
  inRangeY,
  formatInfo
} from '../cl.utils'
// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawCursor
 * @export
 * @class ClDrawCursor
 */
export default class ClDrawCursor {
  /**

   * Creates an instance of ClDrawCursor.
   * @param {Object} father
   * @param {Object} rectMain
   * @param {Object} rectChart
   */
  constructor (father, cfg) {
    initCommonInfo(this, father)
    this.rectFather = father.rectMain
    this.rectMain = cfg.rectMain // 画十字线和边界标签
    this.rectChart = cfg.rectChart // 鼠标有效区域
    this.format = cfg.format || 'cross'

    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo
    
    this.axisYdata = father.axisYdata
    
    this.axisXInfo = father.config.axisX
    this.axisYInfo = father.config.axisY
    this.axisX = father.layout.axisX

    this.context = father.father.moveCanvas.context
    this.onClear()
  }
  /**
   * handle clear
   * @memberof ClDrawCursor
   */
  onClear () {
    _clearRect(this.context, this.rectFather.left, this.rectFather.top,
      this.rectFather.left + this.rectFather.width,
      this.rectFather.top + this.rectFather.height)
  }
  /**
   * paint
   * @param {Object} mousePos
   * @param {Number} valueX
   * @param {Number} valueY
   * @memberof ClDrawCursor
   */
  drawCross(mousePos, valueX, valueY) {
    let txt
    let xx = mousePos.x
    let yy = mousePos.y
    const offX = this.axisPlatform === 'phone' ? 2 * this.scale : -2 * this.scale
    const codeinfo = this.dataLayer.keyInfo[this.linkInfo.curKey].static
    
    // 如果鼠标在本图区域，就画横线信息
    if (inRangeY(this.rectChart, mousePos.y)) {
      if (valueY === undefined) {
        valueY = this.axisYdata.max - (mousePos.y - this.rectChart.top) / this.axisYdata.unitY
      } else {
        yy = (this.axisYdata.max - valueY) * this.axisYdata.unitY + this.rectChart.top
      }

      _drawLineAlone(this.context, this.rectMain.left, yy, this.rectMain.left + this.rectMain.width, yy, this.color.cursor)
      let posX = this.axisPlatform === 'phone' ? 'start' : 'end'

      if (this.axisYInfo.left.display !== 'none') {
        txt = formatInfo(
          valueY,
          this.axisYInfo.left.format,
          codeinfo)
        xx = this.rectMain.left + offX
        _drawTxtRect(this.context, xx, yy, txt, {
          font: this.axisX.font,
          pixel: this.axisX.pixel,
          spaceX: this.axisX.spaceX,
          clr: this.color.txt,
          bakclr: this.color.grid,
          x: posX,
          y: 'middle'
        })
      }
      if (this.axisYInfo.right.display !== 'none') {
        txt = formatInfo(
          valueY,
          this.axisYInfo.right.format,
          codeinfo)
        posX = this.axisPlatform === 'phone' ? 'end' : 'start'
        xx = this.rectMain.left + this.rectMain.width - offX
        _drawTxtRect(this.context, xx, yy, txt, {
          font: this.axisX.font,
          pixel: this.axisX.pixel,
          spaceX: this.axisX.spaceX,
          clr: this.color.txt,
          bakclr: this.color.grid,
          x: posX,
          y: 'middle'
        })
      }
    }

    _drawLineAlone(this.context, mousePos.x, this.rectMain.top, mousePos.x, this.rectMain.top + this.rectMain.height - 1, this.color.cursor)
    if (this.axisXInfo.display !== 'none') {
        xx = mousePos.x
        // console.log('========', valueX, this.father.data.key);
        const th = Math.floor((this.axisX.height - this.axisX.pixel - this.scale) / 2)
        yy = this.rectMain.top + this.rectMain.height + th
        const len = Math.round(_getTxtWidth(this.context, valueX, this.axisX.font, this.axisX.pixel) / 2)
        let posX = 'center'
        if (xx - len < this.rectMain.left + offX) {
            xx = this.rectMain.left + offX;
            posX = 'start'
        }
        if (xx + len > this.rectMain.left + this.rectMain.width - offX) {
            xx = this.rectMain.left + this.rectMain.width - offX
            posX = 'end'
        }
        _drawTxtRect(this.context, xx, yy, valueX, {
            font: this.axisX.font,
            pixel: this.axisX.pixel,
            spaceX: this.axisX.spaceX,
            clr: this.color.txt,
            bakclr: this.color.grid,
            x: posX,
            y: 'top'
        })
    }
  }
  drawHorLine(valueY) {
    _drawLineAlone(this.context, this.rectMain.left, valueY, this.rectMain.left + this.rectMain.width, valueY, this.color.cursor)
  }
  onPaint (mousePos, valueX, valueY) {
    if (typeof this.context._beforePaint === 'function') {
      this.context._beforePaint()
    }
    if (inRangeX(this.rectChart, mousePos.x) === false) return
    this.onClear()
    if (this.format === 'cross') {
        this.drawCross(mousePos, valueX, valueY)
    } 
    else {
        this.drawHorLine(valueY)
    }
    
    if (typeof this.context._afterPaint === 'function') {
      this.context._afterPaint()
    }
  }
}
