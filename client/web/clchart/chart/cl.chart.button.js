/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

import {
    _drawRect,
    _drawVline,
    _drawHline,
    _drawCircle,
    _setLineWidth,
    _drawTxt,
    _fillRect,
    _drawBegin,
    _drawSignCircle,
    _drawSignPlot,
    _setTransColor,
    _drawEnd
} from './cl.draw'
import {
    initCommonInfo,
    checkLayout
} from './cl.chart.init'
import {
    CHART_LAYOUT
} from '../cl.chart.def'
import {
    updateJsonOfDeep
} from '../cl.utils'

const DEFAULT_BUTTON = {
    curIndex: 0,
    visible: true,
    translucent: true, // 是否透明
    status: 'enabled' // disable focused : 热点
}

// ▲▼※★☆○●◎☉√↑←→↓↖↗↘↙‰℃∧∨△□▽♂♀﹡
/**
 * Class representing ClChartButton
 * @export
 * @class ClChartButton
 */
export default class ClChartButton {
    /**

     * Creates an instance of ClChartButton.
     * @param {Object} father
     */
    constructor(father) {
        initCommonInfo(this, father)
    }
    init(cfg, callback) {
        this.callback = callback
        this.rectMain = cfg.rectMain || {
            left: 0,
            top: 0,
            width: 25,
            height: 25
        }
        this.layout = updateJsonOfDeep(cfg.layout, CHART_LAYOUT)
        this.config = updateJsonOfDeep(cfg.config, DEFAULT_BUTTON)

        // If it is not below '+' '-' 'left' 'right', it means to directly display the string
        this.info = cfg.info || {
            drawMethod: 'incr'
        }

        // Make some checks on the configuration
        this.checkConfig()
    }
    /**
     * Check for conflicting configuration changes
     * @memberof ClChartButton
     */
    checkConfig() {
        checkLayout(this.layout)
    }
    setStatus(status) {
        if (this.config.status !== status) {
            this.config.status = status
        }
    }
    /**
     * handle click event
     *
     * @param {Object} event
     * @memberof ClChartButton
     */
    onClick(event) {
        if (!this.config.visible) return
        // if (this.config.status === 'disabled') return
        if (this.info.clickInfo !== undefined && this.info.clickInfo.length > 0) {
            this.config.curIndex++
            this.config.curIndex %= this.info.clickInfo.length
            this.onPaint()
            if (this.config.curIndex >= 0 && this.config.curIndex < this.info.clickInfo.length) {
                this.callback({
                    self: this,
                    info: this.info.clickInfo[this.config.curIndex].value
                })
            }
        }
        else {
            this.callback({
                self: this
            })
        }
        event.break = true
    }
    /**
     * paint buttons
     * @memberof ClChartButton
     */
    onPaint() {
        if (!this.config.visible) return
        _setLineWidth(this.context, this.scale)

        // console.log(this.info, this.config);
        let clr = this.color.button
        if (this.config.status === 'disabled') clr = this.color.grid

        const center = {
            xx: Math.floor(this.rectMain.width / 2),
            yy: Math.floor(this.rectMain.height / 2),
            offset: 4 * this.scale
        }
        // 先画背景
        // 默认仅仅有方框
        _drawBegin(this.context, clr)
        _fillRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back)
        _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)
        _drawEnd(this.context)
        
        // 再画按钮的信息
        if (this.config.status === 'disabled') _drawBegin(this.context, this.color.grid)
        else _drawBegin(this.context, this.color.button)
        switch (this.info.drawMethod) {
            case 'incr':
                _drawVline(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.offset,
                    this.rectMain.top + this.rectMain.height - center.offset)
                _drawHline(this.context, this.rectMain.left + center.offset,
                    this.rectMain.left + this.rectMain.width - center.offset, this.rectMain.top + center.yy)
                break
            case 'decr':
                _drawHline(this.context, this.rectMain.left + center.offset,
                    this.rectMain.left + this.rectMain.width - center.offset, this.rectMain.top + center.yy)
                break
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                _drawTxt(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.yy, info.map,
                    this.layout.title.font, this.layout.title.pixel, this.color.baktxt, {
                        x: 'center',
                        y: 'middle'
                    })
                break
            case '*':
                _drawTxt(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.yy - 2 * this.scale, '*',
                    this.layout.title.font, this.layout.title.pixel, this.color.baktxt, {
                        x: 'center',
                        y: 'middle'
                    })
                break
            case 'left':
            case 'right':
                break
            case 'text':
            default:
                let txt = '*'
                // console.log(this.info, this.config.curIndex);
                if (this.info.clickInfo.length > 0) {
                    txt = this.info.clickInfo[this.config.curIndex].label
                } 
                _drawTxt(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.yy, txt,
                    this.layout.title.font, this.layout.title.pixel, this.color.button, {
                        x: 'center',
                        y: 'middle'
                    })
                break
        }
        _drawEnd(this.context)
    }
}


// const info = this.info.clickInfo[this.config.curIndex]
// switch (this.config.shape) {
//     case 'set':
//         if (this.config.status === 'focused') {
//             clr = this.color.red
//             if (this.config.translucent) clr = _setTransColor(clr, 0.95)
//             _drawSignPlot(this.context, this.rectMain.left + center.xx,
//                 this.rectMain.top + center.xx, {
//                     r: Math.round(this.layout.symbol.size / 2),
//                     clr
//                 }
//             )
//             center.yy = center.xx
//         } else {
//             clr = this.color.vol
//             if (this.config.translucent) clr = _setTransColor(clr, 0.85)
//             _drawSignCircle(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.xx, {
//                 r: Math.round(this.layout.symbol.size / 2),
//                 clr
//             })
//         }
//         break
//     case 'arc':
//         _drawCircle(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.xx, center.xx)
//         break
//     case 'box':
//         _fillRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back)
//         _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)
//         break
//     case 'range':
//         _drawCircle(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.xx, center.xx)
//         break
//     case 'radio':
//         _drawCircle(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.xx, center.xx)
//         break
//     case 'checkbox':
//         _drawCircle(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.xx, center.xx)
//         break
//     default:
// }
// _drawEnd(this.context)
// if (this.config.status === 'disabled') _drawBegin(this.context, this.color.grid)
// else _drawBegin(this.context, this.color.button)
// switch (info.map) {
//     case '+':
//         _drawVline(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.offset,
//             this.rectMain.top + this.rectMain.height - center.offset)
//         _drawHline(this.context, this.rectMain.left + center.offset,
//             this.rectMain.left + this.rectMain.width - center.offset, this.rectMain.top + center.yy)
//         break
//     case '-':
//         _drawHline(this.context, this.rectMain.left + center.offset,
//             this.rectMain.left + this.rectMain.width - center.offset, this.rectMain.top + center.yy)
//         break
//     case '0':
//     case '1':
//     case '2':
//     case '3':
//     case '4':
//     case '5':
//     case '6':
//     case '7':
//     case '8':
//     case '9':
//         _drawTxt(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.yy, info.map,
//             this.layout.title.font, this.layout.title.pixel, this.color.baktxt, {
//                 x: 'center',
//                 y: 'middle'
//             })
//         break
//     case '*':
//         _drawTxt(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.yy - 2 * this.scale, '...',
//             this.layout.title.font, this.layout.title.pixel, this.color.baktxt, {
//                 x: 'center',
//                 y: 'middle'
//             })
//         break
//     case 'left':
//     case 'right':
//         break
//     default:
//         _drawTxt(this.context, this.rectMain.left + center.xx, this.rectMain.top + center.yy, info.map,
//             this.layout.title.font, this.layout.title.pixel, this.color.button, {
//                 x: 'center',
//                 y: 'middle'
//             })
//         break
// }