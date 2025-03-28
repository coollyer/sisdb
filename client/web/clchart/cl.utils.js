/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

//   时间处理函数

/** @module ClTool */

/**
 * add prefix with 0
 * @export
 * @param {Number} v
 * @param {Number} n
 * @return {String}
 */
export function addPreZero(v, n) { // n表示总共几位  (9,2) ---09
    n = n > 9 ? 9 : n
    const s = '000000000' + v
    return s.slice(-1 * n)
}
/**
 * get random text
 * @export
 * @param {Number} size
 * @return {String}
 */
function makeId(size) {
    let text = ''
    const possible = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'
    if (size > 20) size = 20;
    for (let i = 0; i < size; i++) {
        text += possible.charAt(Math.floor(Math.random() * possible.length))
    }

    return text
}
/**
 * get time's millseconds
 * @export
 * @param {any} ttime
 * @return {Number}
 */
export function getMTime(ttime) { // 得到1970-1-1开始的毫秒数
    let mtime, seconds
    if (ttime === undefined) {
        mtime = new Date()
    } else {
        if (typeof ttime === 'string') {
            seconds = parseInt(ttime)
        } else {
            seconds = ttime
        }
        if (!isNaN(seconds)) {
            mtime = new Date(seconds * 1000)
        } else {
            mtime = new Date()
        }
    }
    return mtime
}

// time_t转换成20180101格式
/**
 * format time_t to 20180101
 *
 * @export
 * @param {any} ttime
 * @return {Number}
 */
export function getMsecDate(msec) {
    let mtime = new Date(msec);
    return mtime.getFullYear() * 10000 + (mtime.getMonth() + 1) * 100 + mtime.getDate()
}
export function getMsecMinu(msec) {
    const mtime = new Date(msec);
    return mtime.getHours() * 100 + mtime.getMinutes()
}

export function getDate(ttime) {
    const mtime = getMTime(ttime)
    return mtime.getFullYear() * 10000 + (mtime.getMonth() + 1) * 100 + mtime.getDate()
}
// time_t提取其中的分钟 1030
export function getMinute(ttime) {
    const mtime = getMTime(ttime)
    return mtime.getHours() * 100 + mtime.getMinutes()
}
// 求星期几 0-周日 6-周六
export function getDayWeek(day) {
    const mtime = new Date(Math.floor(day / 10000), Math.floor((day % 10000) / 100) - 1, day % 100)
    return mtime.getDay()
}
// 求星期几 0-周日 6-周六
export function getDayMon(day) {
    const mtime = new Date(Math.floor(day / 10000), Math.floor((day % 10000) / 100) - 1, day % 100)
    return mtime.getMonth() + 1
}
// 日期转换成time_t
function _dayToTTime(day) {
    const mtime = new Date(Math.floor(day / 10000), Math.floor((day % 10000) / 100) - 1, day % 100)
    return mtime / 1000
}
// 得到两个日期间隔的天数
export function getDayGap(beginDay, endDay) {
    return Math.floor((_dayToTTime(endDay) - _dayToTTime(beginDay)) / (24 * 3600))
}

// 格式化time_t为指定字符串
export function fromTTimeToStr(ttime, format, ttimePre) {
    const mtime = getMTime(ttime)
    switch (format) {
        case 'minute':
            if (ttimePre === undefined) {
                return mtime.getHours() + ':' + addPreZero(mtime.getMinutes(), 2)
            } else {
                if (getMinute(ttime) === getMinute(ttimePre)) {
                    return ':' + addPreZero(mtime.getSeconds(), 2)
                } else {
                    return mtime.getHours() + ':' + addPreZero(mtime.getMinutes(), 2)
                }
            }
            case 'datetime':
                return mtime.getFullYear() * 10000 + (mtime.getMonth() + 1) * 100 + mtime.getDate() +
                    '-' + mtime.getHours() + ':' + addPreZero(mtime.getMinutes(), 2)
            default:
                return ''
    }
}

// 分钟转字符串 1500 -- 15:00
export function fromMinuteToStr(minute) {
    return addPreZero(Math.floor(minute / 100), 2).toString() + ':' + addPreZero(minute % 100, 2).toString()
}
// 得到间隔分钟数
export function getMinuteGap(beginMin, endMin) {
    return (Math.floor(endMin / 100) - Math.floor(beginMin / 100)) * 60 + endMin % 100 - beginMin % 100
}

// 偏移分钟数，offset 为分钟数
export function getMinuteOffset(minute, offset) {
    const mincount = Math.floor(minute / 100) * 60 + minute % 100 + offset
    return Math.floor(mincount / 60) * 100 + mincount % 60
}

// 公用无关性的函数集合

export function copyArrayOfDeep(obj) {
    let out
    if (Array.isArray(obj)) {
        out = []
        const len = obj.length
        for (let i = 0; i < len; i++) {
            out[i] = copyArrayOfDeep(obj[i])
        }
    } else {
        out = obj
    }
    return out
}
export function copyJsonOfDeep(obj) {
    let out
    if (obj instanceof Object) { // 数组和文档都为真
        if (Array.isArray(obj)) { // 只有数组为真
            out = copyArrayOfDeep(obj)
        } else {
            out = {}
            for (const key in obj) {
                out[key] = copyJsonOfDeep(obj[key])
            }
        }
    } else {
        out = obj
    }
    return out
}

export function isValue(obj) {
    if (obj === undefined) return false
    // if (isNaN(obj)) return false
    return true
}
// 替换source中存在的对应元素 增加source中不存在的数据
// 例如 source = {a:1,b:2} newobj = {a:5, c:10}
// source = {a:5, b:2, c:10}
export function setJsonOfDeep(source, newobj) {
    console.log(source, newobj);
    if (newobj instanceof Object) { // 数组和文档都为真
        if (!Array.isArray(newobj)) { // 只有数组为真
            for (const key in newobj) {
                if (newobj[key] instanceof Object && !Array.isArray(newobj[key])) {
                    setJsonOfDeep(source[key], newobj[key])
                } else {
                    source[key] = newobj[key]
                    console.log('--', key, source[key], newobj[key]);
                }
            }
        }
    }
}
// obj为子集，生成新的对象，仅仅替换source中存在的对应元素
// 例如 obj = {a:[111],b:2} source = {a:[1,2,3]}
// out = {a:[111,2,3]}
export function updateJsonOfDeep(obj, source) {
    let out
    if (source instanceof Object) {
        if (Array.isArray(source)) {
            out = []
            for (const key in source) {
                out[key] = isValue(obj) && isValue(obj[key]) ?
                    updateJsonOfDeep(obj[key], source[key]) :
                    copyArrayOfDeep(source[key])
            }
        } else {
            out = {}
            for (const key in source) {
                out[key] = isValue(obj) && isValue(obj[key]) ?
                    updateJsonOfDeep(obj[key], source[key]) :
                    copyJsonOfDeep(source[key])
            }
        }
    } else {
        out = isValue(obj) ? obj : source
    }
    return out
}
// // obj为原始集，不生成新的对象，用source中存在的对应元素替换obj的数据
// // 例如 obj = {a:[111],b:2} source = {a:[1,2,3]}
// // out = {a:[111,2,3],b:2}
export function mergeJsonOfDeep(obj, source) {
    const out = updateJsonOfDeep(obj, source)

    for (const key in obj) {
        if (out[key] !== undefined) continue
        if (obj[key] instanceof Object) {
            if (Array.isArray(obj[key])) {
                out[key] = copyArrayOfDeep(obj[key])
            } else {
                out[key] = copyJsonOfDeep(obj[key])
            }
        } else {
            out[key] = obj[key]
        }
    }
    return out
}
// 数组是否为空
export function isEmptyArray(obj) {
    if (obj !== undefined && Array.isArray(obj)) {
        if (obj.length > 0) return false
    }
    return true
}
// 根据offset返回一个新的矩形
// rect:{left,top,width,height}
// offset:{left,top,right,bottom}
export function offsetRect(rect, offset) {
    if (rect === undefined) return {
        left: 0,
        top: 0,
        width: 0,
        height: 0
    }
    if (offset === undefined) return rect
    return {
        left: rect.left + offset.left,
        top: rect.top + offset.top,
        width: rect.width - (offset.left + offset.right),
        height: rect.height - (offset.top + offset.bottom)
    }
}
// 判断点是否在矩形内
export function inRect(rect, point) {
    if (rect === undefined || point === undefined) return false
    if (point.x >= rect.left && point.y >= rect.top &&
        point.x < (rect.left + rect.width) &&
        point.y < (rect.top + rect.height)) {
        return true
    }
    return false
}
// 判断X是否在矩形宽度范围内
export function inRangeX(rect, xx) {
    if (rect === undefined || xx === undefined) return false
    if (xx >= rect.left &&
        xx < (rect.left + rect.width)) {
        return true
    }
    return false
}
// 判断Y是否在矩形高度范围内
export function inRangeY(rect, yy) {
    if (rect === undefined || yy === undefined) return false
    if (yy >= rect.top &&
        yy < (rect.top + rect.height)) {
        return true
    }
    return false
}
// 判断 v 是否在数组arr中, 比in准确
export function inArray(v, arr) {
    if (arr.indexOf(v) < 0) return false
    return true
}
// 求数组a和b的交集
export function intersectArray(a, b) {
    const result = []
    for (let ai = 0; ai < a.length; ai++) {
        for (let bi = 0; bi < b.length; bi++) {
            if (a[ai] === b[bi]) {
                result.push(a[ai])
                break
            }
        }
    }
    return result
}
// 求数组a和b的并集,去掉重复的元素
export function mergeArray(a, b) {
    const result = []
    for (let ai = 0; ai < a.length; ai++) {
        result.push(a[ai])
    }
    for (let bi = 0; bi < b.length; bi++) {
        if (!inArray(b[bi], result)) {
            result.push(b[bi])
        }
    }
    return result
}
// 不生成新的数组 把incrs添加到 source 中 imode 'after' 从后面增加 'before' 从前面增加
export function incrArray(source, incrs, imode) {
    if (imode === undefined) imode = 'after'
    if (imode === 'before') {
        for (let bi = incrs.length - 1; bi >= 0; bi--) {
            source.unshift(incrs[bi])
        }
    } else {
        for (let bi = 0; bi < incrs.length; bi++) {
            source.push(incrs[bi])
        }
    }
}

// 格式化百分比
export function formatRate(value, zero) {
    if (value === undefined || isNaN(value) || zero === undefined) return '--'
    if (typeof value === 'string') value = parseFloat(value)

    let result = Math.abs((value - zero) / zero * 100)
    result = result.toFixed(2) + '%'
    return result // 10.20%
}
export function formatInteger(value) {
    if (value === undefined || isNaN(value)) return '0'
    result = value.toFixed(0)
    return String(result)
}
export function formatFloat(value, coindot, maxlen) {
    if (value === undefined || isNaN(value)) return '0'
    let result = value
    if (coindot === undefined || coindot < 0 || coindot > 10) coindot = 0

    if (value > -0.000000001 && value < 0.000000001) {
        return '0.0'
    }
    result = result.toFixed(coindot)
    maxlen--
    if (maxlen === undefined || maxlen < 4) return result
    return result.substr(0, maxlen)
}

// 格式化成交量
export function formatVolume(value, volunit) {
    if (value === undefined || isNaN(value)) return '--'
    if (typeof value === 'string') value = parseFloat(value)

    if (volunit === undefined) volunit = 1
    let result = value * volunit

    if (result > 100000000000) result = (result / 100000000).toFixed(0) + '亿'
    else if (result > 10000000000) result = (result / 100000000).toFixed(1) + '亿'
    else if (result > 1000000000) result = (result / 100000000).toFixed(2) + '亿'
    else if (result > 100000000) result = (result / 100000000).toFixed(3) + '亿'
    else if (result > 10000000) result = (result / 10000).toFixed(0) + '万'
    else if (result > 1000000) result = (result / 10000).toFixed(1) + '万'
    else if (result > -0.000000001 && result < 0.000000001) result = '--'
    else result = result.toFixed(0)
    return String(result)
}

// 格式化资金 有负数
export function formatMoney(value, mnyunit) {
    if (value === undefined || isNaN(value)) return '--'
    if (typeof value === 'string') value = parseFloat(value)

    if (mnyunit === undefined) mnyunit = 1
    let result = value * mnyunit

    // if (result > 100000000000) result = (result / 100000000).toFixed(0) + '亿'
    if (Math.abs(result) > 10000000000) result = (result / 100000000).toFixed(0) + '亿'
    else if (Math.abs(result) > 1000000000) result = (result / 100000000).toFixed(1) + '亿'
    else if (Math.abs(result) > 100000000) result = (result / 100000000).toFixed(2) + '亿'
    // else if (Math.abs(result) > 10000000) result = (result / 10000).toFixed(0) + '万'
    else if (Math.abs(result) > 1000000) result = (result / 10000).toFixed(0) + '万'
    else if (Math.abs(result) < 0.000000001) result = '--'
    else result = result.toFixed(0)

    return String(result)
}
// 格式化价格 decimal为小数点，limit为最大长度[4,10]，
export function formatPrice(value, coindot, limit, isopen) {
    if (value === undefined || isNaN(value)) return '--'
    // if (typeof value === 'string') value = parseFloat(value);
    if (coindot === undefined) coindot = 2

    let result = value
    if (coindot === undefined || coindot < 0 || coindot > 10) coindot = 0

    if (value > -0.000000001 && value < 0.000000001 && !isopen) {
        return '--'
    }
    result = result.toFixed(coindot)

    if (limit === undefined || limit < 4) return result
    return result.substr(0, limit)
}
// 格式化时间
export function formatShowTime(key, value, minute) {
    let out = value
    switch (key) {
        case 'M5':
        case 'M15':
        case 'M30':
        case 'M60':
            out = fromTTimeToStr(value, 'datetime')
            break
        case 'MINUTE':
            if (minute === undefined) {
                out = fromTTimeToStr(value, 'minute')
            } else {
                out = fromMinuteToStr(minute)
            }
            break
        case 'MDAY':
            out = fromTTimeToStr(value, 'minute')
            break
    }
    return out
}

export function formatInfo(value, format, stkinfo) {
    let out
    if (format === 'rate') {
        out = formatRate(value, stkinfo.agoprc)
    } else {
        console.log(format, value);
        if (format === 'price') {
            out = formatPrice(value, stkinfo.coindot, 7)
        } else if (format === 'float') {
            out = formatFloat(value, stkinfo.coindot, 7)
        } else if (format === 'money') {
            out = formatMoney(value, stkinfo.mnyunit)
        } else {
            out = formatVolume(value, stkinfo.volunit)
        }
    }
    return out
}

// 数据转换函数集合