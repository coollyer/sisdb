/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

/** @module SystemConfig */

import {
    copyJsonOfDeep
} from '../cl.utils'
import {
    _globalUserClassDefine
} from '../plugins/cl.register'
import ClChartLine from './cl.chart.line'
import ClChartMinute from './cl.chart.minute'
import ClChartBoard from './cl.chart.board'
import ClChartTable from './cl.chart.table'

import * as drawClass from './cl.draw'

/**
 * @constant
 */
export const COLOR_WHITE = {
    sys: 'white',
    line: ['#4048cc', '#dd8d2d', '#168ee0', '#ad7eac', '#ff8290', '#ba1215'],
    back: '#eaeaea',
    txt: '#2f2f2f',
    baktxt: '#2f2f2f',
    axis: '#7f7f7f',
    cursor: '#7f7f7f',
    grid: '#cccccc',
    header: '#fafafa',
    red: '#ff6a6c',
    lred: '#b63197',
    green: '#32cb47',
    white: '#7e7e7e',
    vol: '#dd8d2d',
    yellow : '#dd8d2d',
    button: '#888888',
    frame: '#41bfd0',
    select: '#4868c1',
    box: '#ddf4df',
    code: '#3f3f3f'
}
/**
 * @constant
 */
export const COLOR_BLACK = {
    sys: 'black',
    line: ['#efefef', '#fdc104', '#5bbccf', '#ad7eac', '#bf2f2f', '#cfcfcf'],
    back: '#131313',
    txt: '#bfbfbf',
    baktxt: '#2f2f2f',
    axis: '#afafaf',
    cursor: '#afafaf',
    grid: '#373737',
    header: '#232323',
    red: '#ff6a6c',
    lred: '#b63197',
    green: '#6ad36d',
    white: '#9f9f9f',
    vol: '#fdc104',
    yellow: '#fdc104',
    button: '#9d9d9d',
    frame: '#4868c1',
    select: '#162040',
    box: '#373737',
    code: '#41bfd0'
}
/**
 * @constant
 */
export const CHART_WIDTH_MAP = {
    '0': {
        'width': 7.1999969482421875
    },
    '1': {
        'width': 4.8119964599609375
    },
    '2': {
        'width': 7.1999969482421875
    },
    '3': {
        'width': 7.1999969482421875
    },
    '4': {
        'width': 7.1999969482421875
    },
    '5': {
        'width': 7.1999969482421875
    },
    '6': {
        'width': 7.1999969482421875
    },
    '7': {
        'width': 6.563995361328125
    },
    '8': {
        'width': 7.1999969482421875
    },
    '9': {
        'width': 7.1999969482421875
    },
    '.': {
        'width': 3.167999267578125
    },
    ',': {
        'width': 3.167999267578125
    },
    '%': {
        'width': 11.639999389648438
    },
    ':': {
        'width': 3.167999267578125
    },
    ' ': {
        'width': 3.9959869384765625
    },
    'K': {
        'width': 8.279998779296875
    },
    'V': {
        'width': 7.667999267578125
    },
    'O': {
        'width': 9.203994750976562
    },
    'L': {
        'width': 7.055999755859375
    },
    '-': {
        'width': 7.2599945068359375
    },
    '[': {
        'width': 3.9959869384765625
    },
    ']': {
        'width': 3.9959869384765625
    }
}

/**
 * The following several variables must be established when the system is established, which is a common configuration for everyone.
 */
const _chartInitInfo = {
    runPlatform: 'normal', // 'react-native' | 'mina' | 'web'
    axisPlatform: 'web', // 'web' | 'phone'
    eventPlatform: 'web', // 'react-native' | 'mina' | 'web'
    scale: 1, // Set the zoom ratio according to the dpi of different display devices
    standard: 'china', // 'usa' | 'china' Drawing standards to support the United States and China
    sysColor: 'black', // 'white' | 'black'
    charMap: CHART_WIDTH_MAP, // Used to help degrade font width calculations for some platforms that do not support context measures
    mainCanvas: {}, // Main canvas
    moveCanvas: {}, // Cursor canvas
    sysClass: {
        'system': {
            'normal': ClChartLine,
            'chart': ClChartLine,
            'board': ClChartBoard,
            'table': ClChartTable,
            'minute': ClChartMinute,
        }
    }
}
/**
 * set paint standard
 * @export
 * @param {any} standard
 */
export function setStandard(standard) {
    _chartInitInfo.standard = standard || 'china'
}
/**
 * set system color
 * @export
 * @param {String} syscolor drawing theme
 * @returns system color
 */
export function setColor(syscolor) {
    let color = {}
    if (syscolor === 'white') {
        color = copyJsonOfDeep(COLOR_WHITE)
    } else {
        color = copyJsonOfDeep(COLOR_BLACK)
    }
    // The contrast between the rise and fall of the US market and the Chinese market is opposite
    if (_chartInitInfo.standard === 'usa') {
        const clr = color.red
        color.red = color.green
        color.green = clr
    }
    // Update the current system color
    _chartInitInfo.color = color
    return color
}
/**
 * Used to help those platforms that don't support measureText, degraded calculation font span
 * @export
 * @param {Object} context canvas context
 * @param {String} txt text
 * @param {String} font font family
 * @param {Number} pixel font size
 * @return {Number} text width
 */
export function _getOtherTxtWidth(context, txt, font, pixel) {
    const ww = _chartInitInfo.other.width
    const hh = _chartInitInfo.other.height
    drawClass._fillRect(0, 0, ww, hh, '#000')
    drawClass._drawTxt(_chartInitInfo.other.context, 0, 0, txt, font, pixel, '#fff')
    const imgData = _chartInitInfo.other.context.getImageData(0, 0, ww, hh).data
    let width = 0
    for (let i = 0; i < imgData.length; /* i += 4 */) {
        if (imgData.data[i + 0] !== 0 || imgData.data[i + 1] !== 0 ||
            imgData.data[i + 2] !== 0 || imgData.data[i + 3] !== 255) {
            i += 4
            width++
        } else {
            i += 4 * ww
        }
    }
    return width
}
/**
 * set system scale
 * @export
 * @param {Object} canvas
 * @param {Number} scale
 * @return {Object}
 */
export function setScale(canvas, scale) {
    canvas.width = canvas.clientWidth * scale
    canvas.height = canvas.clientHeight * scale
    return {
        width: canvas.width,
        height: canvas.height
    }
}
/**
 * init system
 * @export
 * @param {Object} cfg system config
 * @return {Object} all system config
 */
export function _initChartInfo(cfg) {
    if (cfg === undefined) return
    for (const key in _chartInitInfo) {
        _chartInitInfo[key] = cfg[key] || _chartInitInfo[key]
    }
    _chartInitInfo.mainCanvas.canvas = cfg.mainCanvas.canvas
    _chartInitInfo.mainCanvas.context = cfg.mainCanvas.canvas.getContext('2d')
    _chartInitInfo.mainCanvas.context.charMap = _chartInitInfo.charMap
    _chartInitInfo.moveCanvas.canvas = cfg.moveCanvas.canvas
    _chartInitInfo.moveCanvas.context = cfg.moveCanvas.canvas.getContext('2d')
    _chartInitInfo.moveCanvas.context.charMap = _chartInitInfo.charMap

    _chartInitInfo.eventCanvas = cfg.eventCanvas

    _chartInitInfo.color = setColor(_chartInitInfo.sysColor)

    if (_chartInitInfo.runPlatform === 'normal') {
        if (_chartInitInfo.mainCanvas.canvas !== undefined && _chartInitInfo.scale !== 1) {
            setScale(_chartInitInfo.mainCanvas.canvas, _chartInitInfo.scale)
            setScale(_chartInitInfo.moveCanvas.canvas, _chartInitInfo.scale)
        }
    }
    return _chartInitInfo
}

/**
 * Bind some basic properties when some chart objects are initialized
 * @export
 * @param {Object} chart
 * @param {Object} father
 */
export function initCommonInfo(chart, father) {
    chart.father = father
    chart.context = father.context
    chart.scale = _chartInitInfo.scale
    chart.color = _chartInitInfo.color
    chart.axisPlatform = _chartInitInfo.axisPlatform
    chart.eventPlatform = _chartInitInfo.eventPlatform
}
/**
 * checkout layout
 * @export
 * @param {Object} layout
 */
export function checkLayout(layout) {
    const scale = _chartInitInfo.scale
    layout.margin.top *= scale
    layout.margin.left *= scale
    layout.margin.bottom *= scale
    layout.margin.right *= scale

    layout.offset.top *= scale
    layout.offset.left *= scale
    layout.offset.bottom *= scale
    layout.offset.right *= scale

    layout.title.pixel *= scale
    layout.title.height *= scale
    layout.title.spaceX *= scale
    layout.title.spaceY *= scale

    if (layout.title.height < (layout.title.pixel + layout.title.spaceY + 2 * scale)) {
        layout.title.height = layout.title.pixel + layout.title.spaceY + 2 * scale
    }

    layout.axisX.pixel *= scale
    layout.axisX.width *= scale
    layout.axisX.height *= scale
    layout.axisX.spaceX *= scale

    layout.scroll.pixel *= scale
    layout.scroll.size *= scale
    layout.scroll.spaceX *= scale

    layout.digit.pixel *= scale
    layout.digit.height *= scale
    layout.digit.spaceX *= scale

    if (layout.digit.height < (layout.digit.pixel + 2 * scale)) {
        layout.digit.height = layout.digit.pixel + 2 * scale
    }

    layout.symbol.size *= scale
    layout.symbol.spaceX *= scale
    layout.symbol.spaceY *= scale
}
/**
 * Change mouse style
 * @export
 * @param {String} style
 */
export function changeCursorStyle(style) {
    if (_chartInitInfo.eventPlatform === 'html5') {
        _chartInitInfo.mainCanvas.canvas.style.cursor = style
        _chartInitInfo.moveCanvas.canvas.style.cursor = style
    }
}
/**
 * Get system line color
 * @export
 * @param {Number} index
 * @return {String}
 */
export function getLineColor(index) {
    if (index === undefined) index = 0
    return _chartInitInfo.color.line[index % _chartInitInfo.color.line.length]
}

// 所有的类都在这里定义，不管是系统的还是用户自定义的，

export function _registerPlugins(userName, className, classEntity) {
    if (userName === undefined || _chartInitInfo.sysClass[userName]) {
        _chartInitInfo.sysClass[userName] = {};
    }
    _chartInitInfo.sysClass[userName][className] = classEntity;
    console.log(_chartInitInfo.sysClass);
}

export function _createClass(className, father) {
    let classList = className.split('.');
    if (classList.length < 1) {
        return new _chartInitInfo.sysClass['system']['normal'](father);
    }
    let userName = 'system'
    let entityName = ''
    if (classList.length === 1) {
        if (_chartInitInfo.sysClass['system'][className] === undefined) {
            return new _chartInitInfo.sysClass['system']['normal'](father);
        }
        return new _chartInitInfo.sysClass['system'][className](father);
    }

    userName = classList.shift()
    entityName = classList.toString();
    // 先找系统中的，没有就找user定义的，再没有就调默认的
    if (_chartInitInfo.sysClass[userName] && _chartInitInfo.sysClass[userName][entityName]) {
        return new _chartInitInfo.sysClass[userName][entityName](father);
    } else if (_globalUserClassDefine[userName] && _globalUserClassDefine[userName][entityName]) {
        return new _globalUserClassDefine[userName][entityName](father);
    }
    return new _chartInitInfo.sysClass['system']['normal'](father);
}