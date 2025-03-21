/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClLineRight 的实体定义

import {
  _getTxtWidth,
  _drawTxt
} from './cl.draw'
import {
  findNearTimeToIndex
} from './cl.chart.tools'
import getValue, {
  getSize
} from '../datas/cl.data.tools'
import {
  initCommonInfo
} from './cl.chart.init'
// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawRight
 * @export
 * @class ClDrawRight
 */
export default class ClDrawRight {
  /**

   * Creates an instance of ClDrawRight.
   * @param {Object} father
   * @param {Object} rectMain
   */
  constructor (father, rectMain) {
    initCommonInfo(this, father)
    this.rectMain = rectMain

    this.linkInfo = father.father.linkInfo
    this.source = father.father
    this.symbol = father.layout.symbol
  }
  /**
   * paint
   * @param {String} key
   * @memberof ClDrawRight
   */
  onPaint (key) {
    if (key !== undefined) this.hotKey = key
    this.data = this.source.getData(this.hotKey)
    this.rightData = this.source.getData('RIGHT')
    if (getSize(this.rightData) < 1) return

    const len = _getTxtWidth(this.context, '▲', this.symbol.font, this.symbol.pixel)
    for (let i = 0; i < this.rightData.value.length; i++) {
      const idx = findNearTimeToIndex(this.data, getValue(this.rightData, 'time', i))
      const offset = idx - this.linkInfo.minIndex
      const xx = this.rectMain.left + offset * (this.linkInfo.unitX + this.linkInfo.spaceX) + Math.floor(this.linkInfo.unitX / 2)
      if (xx < this.rectMain.left) {
        continue
      }
      let clr = this.color.button
      if (this.linkInfo.rightMode !== 'none') clr = this.color.vol
      _drawTxt(this.context, xx - Math.floor(len / 2), this.rectMain.top + this.rectMain.height - this.symbol.pixel - this.symbol.spaceY,
        '▲', this.symbol.font, this.symbol.pixel, clr, { y: 'top' })
    }
  }
}
