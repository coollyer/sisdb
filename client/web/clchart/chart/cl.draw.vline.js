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
  _drawLineAlone,
  _drawmoveTo,
  _drawlineTo
} from './cl.draw'
import getValue from '../datas/cl.data.tools'
import {
  initCommonInfo,
  getLineColor
} from './cl.chart.init'
/**
 * Class representing ClDrawVLine
 * @export
 * @class ClDrawVLine
 */
export default class ClDrawVLine {
  /**

   * Creates an instance of ClDrawVLine.
   * @param {Object} father
   * @param {Object} rectMain
   */
  constructor (father, rectMain) {
    initCommonInfo(this, father)
    this.rectMain = rectMain
    // this.rectMain = {
    //   left:rectMain.left,
    //   top:rectMain.top,
    //   width:rectMain.width,
    //   height:rectMain.height
    // };
    this.father = father.father
    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo

    this.axisYdata = father.axisYdata
  }
  /**
   * paint
   * @param {String} key
   * @memberof ClDrawVLine
   */
  onPaint (key) {
    if (key !== undefined) this.hotKey = key
    this.data = this.father.getData(this.hotKey)

    if (this.info.labelX === undefined) this.info.labelX = 'time'
    if (this.info.labelY === undefined) this.info.labelY = 'vol'

    let xx, yy, value
    let idx
    let clr
    if (this.info.color === undefined) {
      clr = getLineColor(this.info.colorIndex)
    } else {
      clr = this.color[this.info.color]
    }

    _drawBegin(this.context, clr)
    for (let k = this.linkInfo.minIndex, index = 0; k <= this.linkInfo.maxIndex; k++, index++) {
      // if (getValue(this.data, this.info.labelX, index) < 0) continue;
      xx = this.rectMain.left + Math.floor(index * (this.linkInfo.spaceX + this.linkInfo.unitX))
      value = getValue(this.data, this.info.labelY, k)
      if (value < 0) continue
      yy = this.rectMain.top + Math.round((this.axisYdata.max - value) * this.axisYdata.unitY)
      if (yy < this.rectMain.top) {
        yy = this.rectMain.top + 1
        _drawLineAlone(this.context, xx, this.rectMain.top + this.rectMain.height - 1, xx, yy, this.color.white)
        continue
      }
      _drawmoveTo(this.context, xx, this.rectMain.top + this.rectMain.height - 1)
      _drawlineTo(this.context, xx, yy)
    }
    _drawEnd(this.context)
  }
}
