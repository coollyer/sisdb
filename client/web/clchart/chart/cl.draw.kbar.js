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
    _drawKBar,
    _drawmoveTo,
    _drawlineTo
} from './cl.draw'
import getValue from '../datas/cl.data.tools'
import {
    initCommonInfo
} from './cl.chart.init'

// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawKBar
 * @export
 * @class ClDrawKBar
 */
export default class ClDrawKBar {
    /**

     * Creates an instance of ClDrawKBar.
     * @param {Object} father
     * @param {Object} rectMain
     */
    constructor(father, rectMain) {
        initCommonInfo(this, father)
        this.rectMain = rectMain

        this.father = father.father
        this.dataLayer = father.father.dataLayer
        this.linkInfo = this.dataLayer.linkInfo
        this.keyInfo = this.dataLayer.keyInfo

        this.axisYdata = father.axisYdata

    }
    /**
     * paint
     * @param {String} key
     * @memberof ClDrawKBar
     */
    onPaint(key) {
        if (key !== undefined) this.hotKey = key
        this.data = this.father.getData(this.hotKey)

        const codeinfo = this.keyInfo[this.linkInfo.curKey].static
        console.log(codeinfo, this.rectMain);

        let clr = this.color.red
        _drawBegin(this.context, clr)
        let open, newp
        let agop = codeinfo.agoprc
        for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
            // if (this.linkInfo.unitX < this.scale) {
            //     const xx = this.rectMain.left + (k + 0.5)* (this.linkInfo.unitX + this.linkInfo.spaceX)
            //     const yy = this.rectMain.top + Math.round((this.axisYdata.max - getValue(this.data, 'newp', k)) * this.axisYdata.unitY)
            //     if (k == 0) {
            //         _drawmoveTo(this.context, xx, yy)
            //     }
            //     else {
            //         _drawlineTo(this.context, xx, yy)
            //     }
            //     continue
            // }
            if (idx > 0) agop = getValue(this.data, 'newp', idx - 1)
            open = getValue(this.data, 'open', idx)
            newp = getValue(this.data, 'newp', idx)
            if (open < newp || (open === newp && newp >= agop)) {
                _drawKBar(this.context, {
                        filled: this.color.sys === 'white',
                        index: k,
                        spaceX: this.linkInfo.spaceX,
                        unitX: this.linkInfo.unitX,
                        unitY: this.axisYdata.unitY,
                        maxmin: this.axisYdata,
                        rect: this.rectMain,
                        fillclr: clr
                    },
                    [newp,
                        getValue(this.data, 'maxp', idx),
                        getValue(this.data, 'minp', idx),
                        open
                    ])
            }
        }
        _drawEnd(this.context)
        clr = this.color.green
        _drawBegin(this.context, clr)
        for (let k = 0, idx = this.linkInfo.minIndex; idx <= this.linkInfo.maxIndex; k++, idx++) {
            if (idx > 0) agop = getValue(this.data, 'newp', idx - 1)
            open = getValue(this.data, 'open', idx)
            newp = getValue(this.data, 'newp', idx)
            if (open > newp || (open === newp && newp < agop)) {
                _drawKBar(this.context, {
                        filled: true,
                        index: k,
                        spaceX: this.linkInfo.spaceX,
                        unitX: this.linkInfo.unitX,
                        unitY: this.axisYdata.unitY,
                        maxmin: this.axisYdata,
                        rect: this.rectMain,
                        fillclr: clr
                    },
                    [open,
                        getValue(this.data, 'maxp', idx),
                        getValue(this.data, 'minp', idx),
                        newp
                    ])
            }
        }
        _drawEnd(this.context)
    }
}
