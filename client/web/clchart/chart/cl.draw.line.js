/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClLineKBar 的实体定义

import {
  _drawBegin,
  _drawEnd,
  _drawmoveTo,
  _drawlineTo
} from './cl.draw'
import getValue from '../datas/cl.data.tools'
import {
  initCommonInfo,
  getLineColor
} from './cl.chart.init'
import {
  inRect
} from '../cl.utils'

/**
 * Class representing ClDrawLine
 * @export
 * @class ClDrawLine
 */
export default class ClDrawLine {
  /**

   * Creates an instance of ClDrawLine.
   * @param {Object} father
   * @param {Object} rectMain
   */
  constructor (father, rectMain) {
    initCommonInfo(this, father)
    this.rectMain = rectMain

    this.father = father.father
    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo

    this.axisYdata = father.axisYdata
  }
  /**
   * paint
   * @param {String} key
   * @memberof ClDrawLine
   */
  onPaint (key) {    
    if (key !== undefined) this.hotKey = key
    this.data = this.father.getData(this.hotKey)

    if (this.info.labelX === undefined) this.info.labelX = 'index'
    if (this.info.labelY === undefined) this.info.labelY = 'value'

    let xx, yy
    let clr
    // console.log('=====', this.info);
    if (this.info.color === undefined) {
      clr = getLineColor(this.info.colorIndex)
    } else {
        clr = this.color[this.info.color]
    }
    
    let isBegin = false
    _drawBegin(this.context, clr)
    for (let k = this.linkInfo.minIndex, index = 0; k <= this.linkInfo.maxIndex; k++, index++) {
      // if (getValue(this.data, this.info.labelX, index) < 0) continue;
      xx = this.rectMain.left + (index + 0.5)* (this.linkInfo.unitX + this.linkInfo.spaceX)
      yy = this.rectMain.top + Math.round((this.axisYdata.max - getValue(this.data, this.info.labelY, index)) * this.axisYdata.unitY)
    //   console.log(this.info.skipfd);
      if (this.info.skipfd !== undefined) {
        if (getValue(this.data, this.info.skipfd, index) == 0) {
            isBegin = false
        }
      }
      if (!isBegin) {
        isBegin = inRect(this.rectMain, { x: xx, y: yy })
        if (isBegin) _drawmoveTo(this.context, xx, yy)
        continue
      }
      _drawlineTo(this.context, xx, yy)
    }
    _drawEnd(this.context)
  }
}
