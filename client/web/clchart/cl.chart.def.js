/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

import ClDrawKBar from './chart/cl.draw.kbar'
import ClDrawLine from './chart/cl.draw.line'
import ClDrawRight from './chart/cl.draw.right'
import ClDrawVBar from './chart/cl.draw.vbar'
import ClDrawVLine from './chart/cl.draw.vline'
import ClDrawArea from './chart/cl.draw.area'

/** @module ClChartDef */

/**
 * chart main layout config
 */
export const CHART_LAYOUT = {
    margin: { // 画图区和边界的距离
        left: 0,
        top: 0,
        right: 0,
        bottom: 0
    },
    offset: { // 图形的偏移位置（不包括标题等），margin为整个图形区，offset仅仅是图形
        left: 2,
        top: 2,
        right: 2,
        bottom: 0
    },
    title: { // 标题栏的配置
        pixel: 12, // 字体大小
        height: 18, // 最大高度
        spaceX: 10, // X方向留空
        spaceY: 2, // Y方向留空
        font: 'sans-serif'
    },
    axisX: { // 纵坐标显示的配置
        pixel: 12,
        height: 18,
        width: 50, // 最大宽度
        spaceX: 2,
        font: 'sans-serif'
    },
    scroll: { // 最低下针对曲线的全量游标配置
        pixel: 12,
        size: 15, // 游标块的大小
        spaceX: 10,
        font: 'sans-serif'
    },
    digit: { // 数字显示的字体和配置
        pixel: 13,
        height: 16,
        spaceX: 3,
        // font: 'Roboto Mono'
        font: 'Cousine'
    },
    symbol: { // 符号，比如箭头等的字体和配置
        pixel: 10,
        size: 18,
        spaceX: 3,
        font: 'sans-serif'
    }
}

/**
 * chart order config
 */
export const CHART_BOARD = {
    style: 'normal', // 'tiny' only shows buy one sell one 'normal' 5 orders
    title: {
        display: 'text' // 'text' or 'none','none' does not display btn button text
    }
}

export const CHART_TABLE = {
    // 标题配置
    gridline : {
        horizontal : true,  // 横线
        vertical : true,    // 竖线
        // altermode  :  false,  // 是否交替颜色 显示记录
        // altercolor1 :  '#333', // 
        // altercolor2 :  '#555', // 
    },
    header: {
        align : 'right',
        spaceX: 10,  // X方向最小的留空
        spaceY: 5,  // Y方向留空        
        pixel: 15, // 字体大小
        font: 'sans-serif',
        display: 'text' // 'text' or 'none','none' does not display btn button text
    },
    // 配置信息
    record : {
        align : 'right',
        spaceX  : 5,  // X方向最小的留空
        spaceY  : 3,
        pixel: 14,    // 字体大小
        font: 'sans-serif'
    },
    movecolor : '#ccc',   // 光标的颜色
    fieldlock : 0,  // 按左键时，前面3个字段不动
    curKeyIdx : 0,  
    maxlen : 8,

    scrollHor : {
        width: 20,
        display: false
    },
    scrollVer : {
        width: 20,
        display: true
    },
}

const ZOOM_INFO_DEF = {
    index: 5, // 当前默认缩放索引 实际比例要从 linkInfo 中获取
    list: [0.1, 0.5, 1, 2, 3, 4, 6, 8, 10, 13, 16, 19, 23] // 缩放比例如何变化，
}

export const CHART_AREA = {
    // title: { display: 'none' },
    // scroll: {display: 'none'},
    // zoomInfo: 'none',
    scroll: { // 是否显示下方的游标
        display: 'none' // 'none' does not show yes
    },
    title: { // 标题
        display: 'text', // none does not show btn button text
        label: 'K线' // label information to be displayed
    },
    axisX: {
        lines: 0, // 纵向线有几根，不包括两边的框线
        display: 'none', // 'none' | 'both' | 'block', 'none' does not show, 'both': show both, 'block': displays a value for each block based on lines = display coordinates
        type: 'normal', // 'normal' | 'day1' | 'day5'
        format: 'normal' // 坐标显示的格式，date time datetime normal minute(9:30)
    },
    axisY: {
        lines: 3, // 横向线有几根
        left: { //左边的坐标信息
            display: 'none', // none不显示, both, noupper 不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'none', // 是否有中间值 'before'=前收盘 ‘zero’ 0为中间值
            format: 'price' // 输出数据的格式 rate, price 保留一定小数位 vol 没有小数
        },
        right: { //右边的坐标信息
            display: 'none', // none不显示，noupper 不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'none', // 是否有中间值 'before'=前收盘 ‘zero’ 0为中间值
            format: 'price' // rate, price vol
        }
    },
    // 下面是当前图形的所有需要画的线，一般非特殊的线，直接通过配置就可以得到想要的效果
    lines: [{
        // type: 'l_kbar',
        className: ClDrawArea, // 图按ClDrawKBar类来画
        extremum: { // 如何取极值
            method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
            maxvalue: ['high'], // 参与计算最大值的标签
            minvalue: ['low'] // 参与计算最小值的标签
        }
        // 第一根线默认的key是跟随chart的hotKey变化而变化的，其他线要么自己有数据，要么根据hotKey加上公式计算出自己的key
    }]
}
/**
 * chart kbar layout config
 */
export const CHART_KBAR = {
    zoomInfo: ZOOM_INFO_DEF,
    buttons : [
        {  // 放大
            clickName : 'zoomin',
            drawMethod : 'decr',  // 画+号
            // activecolor : 'txt',
        },
        {  // 缩小 
            clickName : 'zoomout',
            drawMethod : 'incr',  // 画-号
            // activecolor : 'txt',
        },
        {  // 均线轨道
            clickName : 'matrack', 
            drawMethod : 'text',   // 显示文本
            clickInfo : [
                { label : '均', value : 'ma' }, 
                { label : '道', value : 'track' },
            ],
            // activecolor : 'txt',
        },
        {  // 除权
            clickName : 'exright', 
            drawMethod : 'text',   // 显示文本
            clickInfo : [
                { label : '权', value : 'none' }, 
                { label : '前', value : 'forword' },
                { label : '后', value : 'after' },
            ],
            // activecolor : 'txt',
        }
    ],
    scroll: { // 是否显示下方的游标
        display: 'normal' // 'none' does not show normal
    },
    title: { // 标题
        display: 'text', // none does not show btn button text
        label: 'K线' // label information to be displayed
    },
    axisX: {
        lines: 0, // 纵向线有几根，不包括两边的框线
        display: 'none',
        leftX : 'normal',   // 'none' 表示不显示
        rightX : 'normal',  // 'none' 表示不显示
        middleX : 'none',   // 'none' 表示不显示
    },
    axisY: {
        lines: 3, // 横向线有几根
        left: { //左边的坐标信息
            display: 'both', // none不显示, both, noupper 不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'none', // 是否有中间值 'before'=前收盘 ‘zero’ 0为中间值
            format: 'price' // 输出数据的格式 rate, price 保留一定小数位 vol 没有小数
        },
        right: { //右边的坐标信息
            display: 'both', // none不显示，noupper 不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'none', // 是否有中间值 'before'=前收盘 ‘zero’ 0为中间值
            format: 'price' // rate, price vol
        }
    },
    // 下面是当前图形的所有需要画的线，一般非特殊的线，直接通过配置就可以得到想要的效果
    lines: [{
            // type: 'l_kbar',
            className: ClDrawKBar, // 图按ClDrawKBar类来画
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['maxp'], // 参与计算最大值的标签
                minvalue: ['minp']  // 参与计算最小值的标签
            }
            // 第一根线默认的key是跟随chart的hotKey变化而变化的，其他线要么自己有数据，要么根据hotKey加上公式计算出自己的key
        },
        {
            className: ClDrawRight
        },
        {
            className: ClDrawLine,
            info: { // 输出在信息栏目（title）的数据
                txt: ' 5:',
                labelY: 'value', // 从key中获取对应的数据标签 用于显示信息用
                format: 'price'
            },
            formula: { // 数据生成方式，都需要基于基本数据，没有formula表示取绑定的数据
                key: 'DAYM1', // 生成和获取数据的key，必须要定义一个key，否则当你要使用的时候，并不知道取什么样的数据；
                //另外当客户端修改参数或公式时，可以只修改参数和公式，而不修改key，也可以避免垃圾数据过多
                command: `out = this.MA('newp',5)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '10:',
                labelY: 'value',
                format: 'price'
            },
            formula: {
                key: 'DAYM2', // 获取数据的key，
                command: `out = this.MA('newp',10)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '20:',
                labelY: 'value',
                format: 'price'
            },
            formula: {
                key: 'DAYM3', // 获取数据的key，
                command: `out = this.MA('newp',20)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '60:',
                labelY: 'value',
                format: 'price'
            },
            formula: {
                key: 'DAYM4', // 获取数据的key，
                command: `out = this.MA('newp',60)` // 公式只能输出值到out中
            }
        }
    ]
}

/**
 * chart vbar layout config
 */
export const CHART_VBAR = {
    zoomInfo: ZOOM_INFO_DEF,
    title: {
        display: 'text', // none 不显示 btn 按钮 text 文字
        label: 'VOL' // 需要显示的文字信息
    },
    axisX: {
        lines: 0,
        display: 'normal',
        leftX : 'normal',   // 'none' 表示不显示
        rightX : 'normal',  // 'none' 表示不显示
        middleX : 'none',   // 'none' 表示不显示
    },
    axisY: {
        lines: 1,
        left: {
            display: 'nofoot',
            middle: 'none',
            format: 'vol'
        },
        right: {
            display: 'nofoot',
            middle: 'none',
            format: 'vol'
        }
    },
    lines: [{
            className: ClDrawVBar,
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['sumv'], // 参与计算最大值的标签
                minvalue: [0] // 参与计算最小值的标签
            },
            info: {
                showstyle : 'normal',  // 'normal' 正常显示bar 'zero' 以0上下显示
                clrfield : 'clr',  // 从颜色字段获取颜色 red green
                labelY: 'sumv', // 需要显示的变量，从key中获取对应的数据标签
                format: 'vol'
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '5:',
                labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
                format: 'vol'
            },
            formula: {
                key: 'MVOL1', // 获取数据的key，
                command: `out = this.MA('sumv',5)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '10:',
                labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
                format: 'vol'
            },
            formula: {
                key: 'MVOL2', // 获取数据的key，
                command: `out = this.MA('sumv',10)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '20:',
                labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
                format: 'vol'
            },
            formula: {
                key: 'MVOL3', // 获取数据的key，
                command: `out = this.MA('sumv',20)` // 公式只能输出值到out中
            }
        }
    ]
}

export const CHART_KBARANALY = {
    title: {
        display: 'text', // none 不显示 btn 按钮 text 文字
        label: '大单统计' // 需要显示的文字信息
    },
    axisX: {
        lines: 0,
        display: 'normal',  
        leftX : 'normal',   // 'none' 表示不显示 表示从 dataTxt 列中获取 'chunk' 从 showTxt 得到数据
        rightX : 'normal',  // 'none' 表示不显示
        middleX : 'chunk',   // 'none' 表示不显示 
    },
    axisY: {
        lines: 1,
        left: {
            display: 'both',
            middle: 'none',
            format: 'money'
        },
        right: {
            display: 'both',
            middle: 'none',
            format: 'money'
        }
    },
    lines: [
        {
            className: ClDrawVBar,
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['bigm'], // 参与计算最大值的标签
                minvalue: ['bigm']  // 参与计算最小值的标签
            },
            info: {
                showstyle : 'zero',  // 'normal' 正常显示bar 'zero' 以0上下显示
                filled : true,
                // clr_field : 'clr',  // zero 是大于0 为red 小于0 为 green
                showType : '',     // 
                labelX: 'index',
                labelY: 'bigm', // 需要显示的变量，从key中获取对应的数据标签
                format: 'money',
            }
        },
        {
            className: ClDrawLine,
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['value'], // 参与计算最大值的标签
                minvalue: ['value']  // 参与计算最小值的标签
            },
            info: {
                txt: '5:',
                labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
                format: 'money'
            },
            formula: {
                key: 'BIGM1', // 获取数据的key，
                command: `out = this.SUMMA('bigm',5)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['value'], // 参与计算最大值的标签
                minvalue: ['value']  // 参与计算最小值的标签
            },
            info: {
                txt: '10:',
                labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
                format: 'money'
            },
            formula: {
                key: 'BIGM2', // 获取数据的key，
                command: `out = this.SUMMA('bigm',10)` // 公式只能输出值到out中
            }
        },
        {
            className: ClDrawLine,
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['value'], // 参与计算最大值的标签
                minvalue: ['value']  // 参与计算最小值的标签
            },
            info: {
                txt: '20:',
                labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
                format: 'money'
            },
            formula: {
                key: 'BIGM3', // 获取数据的key，
                command: `out = this.SUMMA('bigm',20)` // 公式只能输出值到out中
            }
        },
        // {
        //     className: ClDrawLine,
        //     extremum: { // 如何取极值
        //         method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
        //         maxvalue: ['value'], // 参与计算最大值的标签
        //         minvalue: ['value']  // 参与计算最小值的标签
        //     },
        //     info: {
        //         txt: '60:',
        //         labelY: 'value', // 需要显示的变量，从key中获取对应的数据标签
        //         format: 'money'
        //     },
        //     formula: {
        //         key: 'BIGM4', // 获取数据的key，
        //         command: `out = this.SUMMA('bigm',60)` // 公式只能输出值到out中
        //     }
        // }
    ]
}
/**
 * chart now config
 */
export const CHART_MINUTE = {
    title: {
        display: 'none' // none 不显示 btn 按钮 text 文字
    },
    axisX: {
        lines: 3,
        display: 'none',
        // leftX : 'normal',   // 'none' 表示不显示 'normal' 表示从 dataTxt 列中获取 'chunk' 从 showTxt 得到数据
        // rightX : 'normal',  // 'none' 表示不显示
        // middleX : 'chunk',   // 'none' 表示不显示 
    },
    axisY: {
        lines: 3,
        left: {
            display: 'both', // none不显示，all, noupper不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'before', // 是否有中间值 'before' = 前收盘 ‘zero’ 0为中间值
            format: 'price' // 输出数据的格式 rate, price, vol money
        },
        right: {
            display: 'both', // none不显示，noupper不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'before', // 是否有中间值 'before'=前收盘 ‘zero’ 0为中间值
            format: 'rate' // rate, price vol
        }
    },
    lines: [{
            className: ClDrawLine,
            extremum: { // 如何取极值
                method: 'fixedLeft', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['maxp'], // 参与计算最大值的标签
                minvalue: ['minp'] // 参与计算最小值的标签
            },
            info: {
                labelX: 'index',
                labelY: 'newp',
                skipfd : 'index', // 画线时从新定位
            }
        },
        {
            className: ClDrawLine,
            info: {
                labelX: 'index',
                skipfd : 'index',
            },
            formula: {
                key: 'MINUAVGP', // 获取数据的key，
                command: `out = this.AVGPRC()` // 均价,要根据股票类型做变化
            }
        }
    ]
}

/**
 * min chart volume config
 */
export const CHART_MINUVOL = {
    title: {
        display: 'none' // none 不显示 btn 按钮 text 文字
    },
    axisX: {
        lines: 3,
        display: 'normal',  
        leftX : 'normal',   // 'none' 表示不显示 表示从 dataTxt 列中获取 'chunk' 从 showTxt 得到数据
        rightX : 'normal',  // 'none' 表示不显示
        middleX : 'chunk',   // 'none' 表示不显示 
    },
    axisY: {
        lines: 1,
        left: {
            display: 'nofoot',
            middle: 'none',
            format: 'vol'
        },
        right: {
            display: 'nofoot',
            middle: 'none',
            format: 'vol'
        }
    },
    lines: [{
        className: ClDrawVLine,
        extremum: { // 如何取极值
            method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
            maxvalue: ['curv'], // 参与计算最大值的标签
            minvalue: [0] // 参与计算最小值的标签
        },
        info: {
            labelX: 'index',
            labelY: 'curv',
            color: 'vol'
        }
    }]
}

export const CHART_MINUANALY = {
    title: {
        display: 'text', // none 不显示 btn 按钮 text 文字
        label: '分时大单' // 需要显示的文字信息
    },
    axisX: {
        lines: 3,
        display: 'normal',  
        leftX : 'normal',   // 'none' 表示不显示 表示从 dataTxt 列中获取 'chunk' 从 showTxt 得到数据
        rightX : 'normal',  // 'none' 表示不显示
        middleX : 'chunk',   // 'none' 表示不显示 
    },
    axisY: {
        lines: 1,
        left: {
            display: 'both',
            middle: 'none',
            format: 'money'
        },
        right: {
            display: 'both',
            middle: 'none',
            format: 'money'
        }
    },
    lines: [
        {
            className: ClDrawVBar,
            extremum: { // 如何取极值
                method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['sumbigm', 'curbigm'], // 参与计算最大值的标签
                minvalue: ['sumbigm', 'curbigm']  // 参与计算最小值的标签
            },
            info: {
                txt: '当前:',
                showstyle : 'zero',  // 'normal' 正常显示bar 'zero' 以0上下显示
                // clr_field : 'clr',  // zero 是大于0 为red 小于0 为 green
                showType : '',     // 
                labelX: 'index',
                labelY: 'curbigm', // 需要显示的变量，从key中获取对应的数据标签
                format: 'money',
            }
        },
        {
            className: ClDrawLine,
            info: {
                txt: '累计:',
                labelX: 'index',
                labelY: 'sumbigm', // 需要显示的变量，从key中获取对应的数据标签
                format: 'money',
                color : 'lred'
            },
        },
        // {
        // className: ClDrawVLine,
        // extremum: { // 如何取极值
        //     method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
        //     maxvalue: ['curv'], // 参与计算最大值的标签
        //     minvalue: [0] // 参与计算最小值的标签
        // },
        // info: {
        //     labelX: 'index',
        //     labelY: 'curv',
        //     color: 'vol'
        // }
    ]
}
/**
 * 5day chart config
 */
export const CHART_MDAY = {
    title: {
        display: 'none' // none 不显示 btn 按钮 text 文字
    },
    axisX: {
        lines: 4,
        display: 'none',
        leftX : 'none',   // 'none' 表示不显示
        rightX : 'none',  // 'none' 表示不显示
        middleX : 'chunk',   // 'none' 表示不显示
    },
    axisY: {
        lines: 3,
        left: {
            display: 'both', // none不显示，all, noupper不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'before',
            format: 'price' // 输出数据的格式 rate, price, vol
        },
        right: {
            display: 'both', // none不显示，noupper不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'before',
            format: 'rate' // rate, price vol
        }
    },
    lines: [{
            className: ClDrawLine,
            extremum: { // 如何取极值
                method: 'fixedLeft', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
                maxvalue: ['close'], // 参与计算最大值的标签
                minvalue: ['close'] // 参与计算最小值的标签
            },
            info: {
                labelX: 'time',
                labelY: 'close'
            }
        },
        {
            className: ClDrawLine,
            info: {
                
            },
            formula: {
                key: 'NOWMDAY', // 获取数据的key，
                command: `out = this.AVGPRC()` // 均价,要根据股票类型做变化
            }
        }
    ]
}

/**
 * 5 days chart volume config
 */
export const CHART_MDAYVOL = {
    title: {
        display: 'none' // none 不显示 btn 按钮 text 文字
    },
    axisX: {
        lines: 4,
        display: 'normal',
        leftX : 'none',   // 'none' 表示不显示
        rightX : 'none',  // 'none' 表示不显示
        middleX : 'chunk',   // 'none' 表示不显示
    },
    axisY: {
        lines: 1,
        left: {
            display: 'nofoot', // none不显示，all, noupper不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'none',
            format: 'vol' // 输出数据的格式 rate, price, vol
        },
        right: {
            display: 'nofoot', // none不显示，noupper不显示最上面, nofoot不显示最下面 = 显示坐标
            middle: 'none',
            format: 'vol' // rate, price vol
        }
    },
    lines: [{
        className: ClDrawVLine,
        extremum: { // 如何取极值
            method: 'normal', // fixedLeft fixedRight 上下固定,此时需要取axisY.middle的定义
            maxvalue: ['vol'], // 参与计算最大值的标签
            minvalue: [0] // 参与计算最小值的标签
        },
        info: {
            labelX: 'time',
            labelY: 'vol',
            color: 'vol'
        }
    }]
}

export const CHART_NORMAL = {
    title: {
        display: 'text', // none 不显示 btn 按钮 text 文字
        label: 'NORMAL'
    },
    axisX: {
        lines: 0,
        display: 'none',
        leftX : 'none',   // 'none' 表示不显示
        rightX : 'none',  // 'none' 表示不显示
        middleX : 'none',   // 'none' 表示不显示
    },
    axisY: {
        lines: 1,
        left: {
            display: 'both',
            middle: 'none',
            format: 'price'
        },
        right: {
            display: 'both',
            middle: 'none',
            format: 'price'
        }
    },
    lines: [{
        className: ClDrawLine
    }]
}
