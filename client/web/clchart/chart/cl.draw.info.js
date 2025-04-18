/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClLineKBar 的实体定义

import {
  _fillRect,
  _getTxtWidth,
  _drawTxt,
  _drawLineAlone,
} from './cl.draw'
import {
  initCommonInfo
} from './cl.chart.init'

// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawInfo
 * @export
 * @class ClDrawInfo
 */
export default class ClDrawInfo {
  /**

   * Creates an instance of ClDrawInfo.
   * @param {Object} father
   * @param {Object} rectMain
   * @param {Object} rectMess
   */
  constructor (father, rectMain, rectMess) {
    initCommonInfo(this, father)
    this.rectMain = rectMain
    this.rectMess = rectMess

    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo

    this.title = father.layout.title
    this.titleInfo = father.config.title
  }
  /**
   * paint
   * @param {Object} message
   * @memberof ClDrawInfo
   */
  onPaint (message) {
    if (this.titleInfo.display === 'none' || !this.linkInfo.showCursorInfo) return

    _fillRect(this.context, this.rectMain.left + this.scale, this.rectMain.top + this.scale,
      this.rectMess.left + this.rectMess.width - 2 * this.scale,
      this.rectMain.height - 2 * this.scale, this.color.back)
    
    _drawLineAlone(this.context, this.rectMain.left + this.rectMain.width, this.rectMain.top, 
        this.rectMain.left + this.rectMain.width, this.rectMain.top + this.rectMain.height, this.color.grid)  

    let clr = this.color.txt
    const spaceY = Math.round((this.title.height - this.title.pixel) / 2)
    const yy = this.rectMess.top + spaceY

    if (this.titleInfo.label !== undefined) {
      _drawTxt(this.context, this.rectMain.left + this.scale, yy, this.titleInfo.label,
        this.title.font, this.title.pixel, clr)
    }
    let xx = this.rectMess.left + this.scale
    for (let i = 0; i < message.length; i++) {
        if (message[i].color === undefined) {
            clr = this.color.line[i]
        } else {
            clr = message[i].color
        }
        if (message[i].txt !== undefined) {
            _drawTxt(this.context, xx, yy, message[i].txt, this.title.font, this.title.pixel, clr)
            xx += _getTxtWidth(this.context, message[i].txt, this.title.font, this.title.pixel) + this.title.spaceX
        }
        if (message[i].value === undefined) continue
        _drawTxt(this.context, xx, yy, ' ' + message[i].value, this.title.font, this.title.pixel, clr)
        xx += _getTxtWidth(this.context, ' ' + message[i].value, this.title.font, this.title.pixel) + this.title.spaceX
    }
  }
}
