/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClLineVBar 的实体定义

import {
  _drawBegin,
  _drawEnd,
  _drawHBar
} from './cl.draw'
import getValue from '../datas/cl.data.tools'
import {
  initCommonInfo
} from './cl.chart.init'

// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawVBar
 * @export
 * @class ClDrawVBar
 */
export default class ClDrawArea {
  /**

   * Creates an instance of ClDrawVBar.
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
   * @memberof ClDrawVBar
   */
  getV(idx) {
    return getValue(this.data, 'maxbidm', idx) - getValue(this.data, 'maxaskm', idx)
    // return getValue(this.data, 'minbidm', idx) - getValue(this.data, 'minaskm', idx)
  }

  onPaint (key) {
    if (key !== undefined) this.hotKey = key
    this.data = this.father.getData(this.hotKey)
    console.log("===", this.data, this.linkInfo)
    
    let maxv = 0
    let minv = 0
    let maxp = 50.98
    let minp = 24.06 
    // const force = true;   // 由于定义了没有在后面使用，因此注释掉
    for (let i = 0; i < this.data.value.length; i++) {        
        let vv = this.getV(i)
        maxv = maxv > vv ? maxv : vv
        minv = minv < vv ? minv : vv

        maxp = maxp > this.data.value[i][0] ? maxp : this.data.value[i][0]
        minp = minp < this.data.value[i][0] ? minp : this.data.value[i][0]
    } // for
    this.axisYdata.maxv = maxv
    this.axisYdata.minv = minv
    this.axisYdata.maxp = maxp
    this.axisYdata.minp = minp
    this.axisYdata.unitX = (this.rectMain.width - 2) / (this.axisYdata.maxv - this.axisYdata.minv) // 一个单位价位多少像素
    this.axisYdata.unitY = (this.rectMain.height - 40) / (this.axisYdata.maxp - this.axisYdata.minp) 
    
    _drawBegin(this.context, this.color.red)
    for (let idx = 0; idx < this.data.value.length; idx++) {
        let vv = this.getV(idx)
        if (vv > 0) 
            {
            _drawHBar(this.context, {
            filled: true,
            index : idx,
            spaceX: 3,
            spaceY: 0.16 * this.axisYdata.unitY,
            unitX: this.axisYdata.unitX,
            unitY: this.axisYdata.unitY,
            maxmin: this.axisYdata,
            rect: this.rectMain,
            fillclr: this.color.red
            },
            vv,
            getValue(this.data, 'newp', idx))
        }
    }
    _drawEnd(this.context)
    _drawBegin(this.context, this.color.green)
    for (let idx = 0; idx < this.data.value.length; idx++) {
        let vv = this.getV(idx)
        if (vv < 0) 
        {
            _drawHBar(this.context, {
            filled: true,
            index : idx,
            spaceX: 3,
            spaceY: 0.16 * this.axisYdata.unitY,
            unitX: this.axisYdata.unitX,
            unitY: this.axisYdata.unitY,
            maxmin: this.axisYdata,
            rect: this.rectMain,
            fillclr: this.color.green
            },
            vv,
            getValue(this.data, 'newp', idx))
        }
    }
    _drawEnd(this.context)
  }
}
