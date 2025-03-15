/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClLineAxisX 的实体定义

import {
  // _drawLineAlone,
  _drawTxt,
  _fillRect
} from './cl.draw'
import {
  initCommonInfo
} from './cl.chart.init'

// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawAxisX
 * @export
 * @class ClDrawAxisX
 */
export default class ClDrawAxisX {
  /**

   * Creates an instance of ClDrawAxisX.
   * @param {Object} father
   * @param {Object} rectMain
   */
  constructor (father, rectMain) {
    initCommonInfo(this, father)
    this.rectMain = rectMain

    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo

    this.axisXinfo = this.dataLayer.axisXinfo

    this.axisX = father.config.axisX

    this.text = father.layout.axisX
  }
  /**
   * pain axisx
   * @memberof ClDrawAxisX
   */
  onPaint () {
    if (this.axisX.display === 'none') return

    const yy = this.rectMain.top + this.rectMain.height / 2 - this.scale

    if (this.axisX.leftX !== 'none') {
        const xx = this.rectMain.left + this.text.spaceX
        const value = this.axisXinfo.dataTxt[this.linkInfo.minIndex]
        if (value !== undefined) {
            _drawTxt(this.context, xx, yy, value,
              this.text.font, this.text.pixel, this.color.axis, {
                y: 'middle'
              })
        }
    }
    if (this.axisX.rightX !== 'none') {
        const xx = this.rectMain.left + this.rectMain.width - 3
        const value = this.axisXinfo.dataTxt[this.linkInfo.maxIndex]
        if (value !== undefined) {
            _drawTxt(this.context, xx, yy, value,
              this.text.font, this.text.pixel, this.color.axis, {
                y: 'middle',
                x: 'end'
              })
        }
    }
    if (this.axisX.middleX !== 'none') {
        const chunks = this.axisXinfo.showTxt.length
        if (chunks > 0) {
            const spaceX = this.rectMain.width / chunks
            for (let index = 0; index < chunks; index++) {
                const xx = this.rectMain.left + spaceX / 2 + spaceX * index
                _drawTxt(this.context, xx, yy, this.axisXinfo.showTxt[index],
                    this.text.font, this.text.pixel, this.color.axis, { y: 'middle', x: 'center' })
            }
        }
    }
  }
}
