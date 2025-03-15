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
    _drawVBar,
    _drawVBarZero,
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
export default class ClDrawVBar {
    /**

     * Creates an instance of ClDrawVBar.
     * @param {Object} father
     * @param {Object} rectMain
     */
    constructor(father, rectMain) {
        initCommonInfo(this, father)
        this.rectMain = rectMain

        this.father = father.father
        this.dataLayer = father.father.dataLayer
        this.linkInfo = this.dataLayer.linkInfo

        this.axisYdata = father.axisYdata
    }
    drawZero() {
        _drawBegin(this.context, this.color.red)
        for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
            const value = getValue(this.data, this.info.labelY, idx)
            if (value >= 0.000001) {
                _drawVBarZero(this.context, {
                        filled: this.info.filled || this.color.sys === 'white',
                        index: k,
                        spaceX: this.linkInfo.spaceX,
                        unitX: this.linkInfo.unitX,
                        unitY: this.axisYdata.unitY,
                        maxmin: this.axisYdata,
                        rect: this.rectMain,
                        fillclr: this.color.red
                    },
                    value)
            }
        }
        _drawEnd(this.context)
        _drawBegin(this.context, this.color.green)
        for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
            const value = getValue(this.data, this.info.labelY, idx)
            if (value < -0.000001) {
                _drawVBarZero(this.context, {
                        filled: true,
                        index: k,
                        spaceX: this.linkInfo.spaceX,
                        unitX: this.linkInfo.unitX,
                        unitY: this.axisYdata.unitY,
                        maxmin: this.axisYdata,
                        rect: this.rectMain,
                        fillclr: this.color.green
                    },
                    value)
            }
        }
        _drawEnd(this.context)
    }
    drawNormal() {
        if (this.info.clrfield === undefined) {
            _drawBegin(this.context, this.color.vol)
            for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
                _drawVBar(this.context, {
                        filled: this.color.sys === 'white',
                        index: k,
                        spaceX: this.linkInfo.spaceX,
                        unitX: this.linkInfo.unitX,
                        unitY: this.axisYdata.unitY,
                        maxmin: this.axisYdata,
                        rect: this.rectMain,
                        fillclr: this.color.vol
                    },
                    getValue(this.data, this.info.labelY, idx))
            }
            _drawEnd(this.context)
        } else {
            _drawBegin(this.context, this.color.red)
            for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
                if (getValue(this.data, this.info.clrfield, idx) === 'red') {
                    _drawVBar(this.context, {
                            filled: this.info.filled || this.color.sys === 'white',
                            index: k,
                            spaceX: this.linkInfo.spaceX,
                            unitX: this.linkInfo.unitX,
                            unitY: this.axisYdata.unitY,
                            maxmin: this.axisYdata,
                            rect: this.rectMain,
                            fillclr: this.color.red
                        },
                        getValue(this.data, this.info.labelY, idx))
                }
            }
            _drawEnd(this.context)
            _drawBegin(this.context, this.color.green)
            for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
                if (getValue(this.data, this.info.clrfield, idx) === 'green') {
                    _drawVBar(this.context, {
                            filled: true,
                            index: k,
                            spaceX: this.linkInfo.spaceX,
                            unitX: this.linkInfo.unitX,
                            unitY: this.axisYdata.unitY,
                            maxmin: this.axisYdata,
                            rect: this.rectMain,
                            fillclr: this.color.green
                        },
                        getValue(this.data, this.info.labelY, idx))
                }
            }
            _drawEnd(this.context)
        }
    }
    onPaint(key) {
        if (key !== undefined) this.hotKey = key
        this.data = this.father.getData(this.hotKey)
        console.log(this.info);
        if (this.info == undefined) return

        if (this.info.showstyle === 'zero') {
            this.drawZero()
        } else {
            this.drawNormal()
        }
    }
}
