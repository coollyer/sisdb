/**
 * Copyright (c) 2018-present clchart Contributors.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 *
 */

// 以下是 ClDrawSeer 的实体定义

// case 1: return '未生效';
// case 2: return '股票停牌';
// case 20: return '股东增持';
// case 100: return '已生效(1 -> 100)';
// case 101: return '已成功(100 -> 101)';
// case 102: return '已失败(100 -> 102)';
// case 200: return '未成功已结束(100 -> 200)';
// case 201: return '成功已结束(101 -> 201)';
// case 202: return '失败已结束(102 -> 202)';
// case 300: return '未生效已结束(1/2 -> 300)';
// case 400: return '错误';

import {
  _drawSignCircle,
  _setTransColor,
  _fillRect,
  _drawSignHLine,
  _drawSignVLine
} from '../chart/cl.draw'
import {
  findNearTimeToIndex
} from '../chart/cl.chart.tools'
import getValue, {
  // getZipDay,
  getExrightPriceRange
} from '../datas/cl.data.tools'
import {
  initCommonInfo
} from '../chart/cl.chart.init'
import {
  inArray,
  inRangeY,
  intersectArray,
  // copyArrayOfDeep,
  formatPrice,
  copyArrayOfDeep
} from '../cl.utils'
import {
  FIELD_DATE
} from '../cl.data.def'

// 预测信息定义
export const FIELD_SEER = {
  // code: 0,
  start: 1, // 开始日期
  period: 2, // 预期周期
  buy: 3, // 买入价
  target: 4, // 目标价
  stoploss: 5, // 止损价
  status: 6, // 当前状态
  // stopdate: 7, // 停止日期 正在进行的为0
  // highdate: 8,  // 最高价格日期
  // lowdate: 9,    // 最低价格日期
  // surplus: 10, // 剩余天数
  uid: 11, // 预测的id
  focused: 12 // 是否热点
}

export const FIELD_SEER_HOT = {
  uid: 0
}

// 创建时必须带入父类，后面的运算定位都会基于父节点进行；
// 这个类仅仅是画图, 因此需要把可以控制的rect传入进来
/**
 * Class representing ClDrawSeer
 * @export
 * @class ClDrawSeer
 */
export default class ClDrawSeer {
  /**
   * Creates an instance of ClDrawSeer.
   * @param {Object} father
   * @param {Object} rectMain
   * @constructor
   */
  constructor (father, rectMain) {
    initCommonInfo(this, father)
    this.rectMain = rectMain

    this.father = father.father
    this.dataLayer = father.father.dataLayer
    this.linkInfo = this.dataLayer.linkInfo
    this.static = this.dataLayer.keyInfo[this.linkInfo.curKey].static

    this.axisYdata = father.axisYdata
    
    this.layout = father.layout
  }

  /**
   * get seer position
   * @param {Number} index
   * @param {Number} nowprice
   * @return {Object}
   * @memberof ClDrawSeer
   */
  getSeerPos (index, nowprice) {
    const offset = index - this.linkInfo.minIndex
    if (offset < 0 || index > this.linkInfo.maxIndex) {
      return {
        visible: false
      }
    }
    // 不在视线内就不画
    const xx = this.rectMain.left + offset * (this.linkInfo.unitX + this.linkInfo.spaceX) + Math.floor(this.linkInfo.unitX / 2)
    let price = nowprice
    if (nowprice === undefined) {
      price = getValue(this.data, 'close', index)
    }
    const yy = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
    return {
      visible: true,
      xx,
      yy
    }
  }
  /**
   * draw hot seer
   * @param {Number} no
   * @memberof ClDrawSeer
   */
  drawHotSeer (no) {
    let idx = findNearTimeToIndex(this.data, getValue(this.sourceSeer, 'start', no), 'time', 'forword')
    if (idx === -1) idx = this.linkInfo.maxIndex
    const offset = idx - this.linkInfo.minIndex
    if (offset < 0) return // 不在视线内就不画

    const xx = this.rectMain.left + offset * (this.linkInfo.unitX + this.linkInfo.spaceX) + Math.floor(this.linkInfo.unitX / 2)

    const status = getValue(this.sourceSeer, 'status', no)
    const startPrice = getValue(this.sourceSeer, 'buy', no)
    let price = startPrice

    let yy = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)

    let startTxt = this.linkInfo.showCursorInfo ? '买点' : '买点:' + formatPrice(price, this.static.coindot)
    if (startPrice === 0) {
      startTxt = '停牌中' // 停牌期间预测的股票
      price = getValue(this.data, 'close', idx)
      yy = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
    }
    let color = this.color.vol
    if (inArray(status, [1, 20, 300])) {
      color = this.color.txt
    } else if (inArray(status, [2])) {
      startTxt = '停牌中'
    }
    _drawSignHLine(this.context, {
      linew: this.scale,
      xx,
      yy,
      right: this.rectMain.left + this.rectMain.width + this.layout.offset.right - 2 * this.scale,
      clr: color,
      bakclr: this.color.back,
      font: this.layout.title.font,
      pixel: this.layout.title.pixel,
      spaceX: 4 * this.scale,
      spaceY: 3 * this.scale,
      x: 'start',
      y: 'middle'
    }, [{
      txt: startTxt,
      set: 100,
      display: !this.linkInfo.showCursorInfo
    }])

    const period = getValue(this.sourceSeer, 'period', no)
    const surplus = period - (this.linkInfo.maxIndex - idx)

    let txt = ' 周期:[' + period + '天]'
    if (surplus > 0) txt += ' 剩余:[' + surplus + '天]'

    _drawSignVLine(this.context, {
      linew: this.scale,
      xx,
      yy,
      left: this.rectMain.left,
      bottom: this.rectMain.top + this.rectMain.height + this.layout.offset.bottom + 2 * this.scale,
      clr: color,
      bakclr: this.color.back,
      font: this.layout.title.font,
      pixel: this.layout.title.pixel,
      spaceX: 2 * this.scale,
      paceY: 2 * this.scale
    }, [{
      txt: 'arc',
      set: 0,
      minR: 0,
      maxR: 3 * this.scale,
      display: true
    },
    {
      txt: getValue(this.sourceSeer, 'start', no) + txt,
      set: 100,
      display: true
    }
    ])
    this.drawTransRect(this.rectMain.left, xx)
    const xxRight = xx + period * (this.linkInfo.spaceX + this.linkInfo.unitX)
    this.drawTransRect(xxRight, this.rectMain.left + this.rectMain.width)
    if (startPrice === 0) return

    let infos
    price = getValue(this.sourceSeer, 'stoploss', no)
    let yl = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
    if (yl - yy > 1.5 * this.layout.title.pixel) {
      infos = [{
        txt: 'arc',
        set: 0,
        minR: 0,
        maxR: 2 * this.scale,
        display: true
      },
      {
        txt: this.linkInfo.showCursorInfo ? '止损' : '止损:' + formatPrice(price, this.static.coindot),
        set: 100,
        display: !this.linkInfo.showCursorInfo
      }
      ]
    } else {
      infos = [{
        txt: 'arc',
        set: 0,
        minR: 0,
        maxR: 2 * this.scale,
        display: true
      },
      {
        txt: 'arc',
        set: 100,
        minR: 0,
        maxR: 1 * this.scale,
        display: true
      }
      ]
    }
    if (inRangeY(this.rectMain, yy)) {
      _drawSignHLine(this.context, {
        linew: this.scale,
        xx,
        yy: yl,
        right: this.rectMain.left + this.rectMain.width + this.layout.offset.right - 2 * this.scale,
        clr: this.color.green,
        bakclr: this.color.back,
        font: this.layout.title.font,
        pixel: this.layout.title.pixel,
        spaceX: 4 * this.scale,
        spaceY: 3 * this.scale,
        x: 'start',
        y: 'middle'
      }, infos)
    }

    price = getValue(this.sourceSeer, 'target', no)
    yl = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
    if (yy - yl > 1.5 * this.layout.title.pixel) {
      infos = [{
        txt: 'arc',
        set: 0,
        minR: 0,
        maxR: 2 * this.scale,
        display: true
      },
      {
        txt: this.linkInfo.showCursorInfo ? '目标' : '目标:' + formatPrice(price, this.static.coindot),
        set: 100,
        display: !this.linkInfo.showCursorInfo
      }
      ]
    } else {
      infos = [{
        txt: 'arc',
        set: 0,
        minR: 0,
        maxR: 2 * this.scale,
        display: true
      },
      {
        txt: 'arc',
        set: 100,
        minR: 0,
        maxR: 2 * this.scale,
        display: true
      }
      ]
    }
    if (inRangeY(this.rectMain, yy)) {
      _drawSignHLine(this.context, {
        linew: this.scale,
        xx,
        yy: yl,
        right: this.rectMain.left + this.rectMain.width + this.layout.offset.right - 2 * this.scale,
        clr: this.color.red,
        bakclr: this.color.back,
        font: this.layout.title.font,
        pixel: this.layout.title.pixel,
        spaceX: 4 * this.scale,
        spaceY: 3 * this.scale,
        x: 'start',
        y: 'middle'
      }, infos)
    }

    const stop = getValue(this.sourceSeer, 'stop', no)
    // 100 进行中
    if (inArray(status, [101, 102, 200, 201, 202, 300])) {
      const stopIdx = findNearTimeToIndex(this.data, stop, 'time', 'forword')
      const stopOffset = stopIdx - this.linkInfo.minIndex
      const stopX = this.rectMain.left + stopOffset * (this.linkInfo.unitX + this.linkInfo.spaceX) + Math.floor(this.linkInfo.unitX / 2)
      if (stopX > this.rectMain.left && stopX < this.rectMain.left + this.rectMain.width - 4 * this.scale) {
        color = this.color.vol
        price = getValue(this.sourceSeer, 'buy', no)
        yl = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
        if (inArray(status, [102, 202])) {
          color = this.color.green
          price = getValue(this.sourceSeer, 'stoploss', no)
          yl = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
        } else if (inArray(status, [101, 201])) {
          color = this.color.red
          price = getValue(this.sourceSeer, 'target', no)
          yl = this.rectMain.top + Math.round((this.axisYdata.max - price) * this.axisYdata.unitY)
        } else if (inArray(status, [300])) {
          color = this.color.txt
        }
        _drawSignCircle(this.context, stopX, yl, {
          r: 3 * this.scale,
          clr: color
        }
          // { r: 2 * this.scale, clr: this.color.back },
        )
      }
    }
  }
  /**
   * filter seer
   * @return {Array}
   * @memberof ClDrawSeer
   */
  filterSeer () {
    const out = {}
    for (let i = 0; i < this.sourceSeer.value.length; i++) {
      const curDate = getValue(this.sourceSeer, 'start', i)
      let index = findNearTimeToIndex(this.data, curDate, 'time', 'forword')
      if (index === -1) index = this.linkInfo.maxIndex
      if (out[index] === undefined) {
        out[index] = {
          nos: [],
          uids: []
        }
      }
      out[index].name = 'SEER' + index
      out[index].date = getValue(this.data, 'time', index)
      out[index].nos.push(i)
      out[index].uids.push(getValue(this.sourceSeer, 'uid', i))
      // 这里判断当前节点是否存在热点
      if (inArray(getValue(this.sourceSeer, 'uid', i), this.hotSeer.value)) {
        out[index].focused = true
        out[index].hotIdx = i
      }
    }
    return out
  }
  /**
   * before location
   * @return {Number}
   * @memberof ClDrawSeer
   */
  beforeLocation () {
    this.linkInfo.rightMode = 'forword'
    this.data = this.source.getData(this.father.hotKey)
    const lastDate = this.data.value[this.data.value.length - 1][this.data.fields.time]

    this.sourceSeer = this.source.getData('SEER')
    this.hotSeer = this.source.getData('SEERHOT')

    if (this.sourceSeer.value.length < 1) return 0
    if (this.hotSeer === undefined) {
      this.hotSeer = {
        value: [getValue(this.sourceSeer, 'uid', 0)]
      }
    }
    if (this.sourceSeer.value.length < 1) return 0
    // 求最大最小日期
    const maxmin = {
      max: getValue(this.sourceSeer, 'start', 0),
      min: getValue(this.sourceSeer, 'start', this.sourceSeer.value.length - 1)
    }
    // 除权处理
    const rights = this.source.getData('RIGHT')
    if (rights !== undefined) {
      const lastval = copyArrayOfDeep(this.sourceSeer.value)
      for (let i = 0; i < lastval.length; i++) {
        lastval[i][FIELD_SEER.buy] =
          getExrightPriceRange(maxmin.min, lastDate, lastval[i][FIELD_SEER.buy], rights.value)
        lastval[i][FIELD_SEER.target] =
          getExrightPriceRange(maxmin.min, lastDate, lastval[i][FIELD_SEER.target], rights.value)
        lastval[i][FIELD_SEER.stoploss] =
          getExrightPriceRange(maxmin.min, lastDate, lastval[i][FIELD_SEER.stoploss], rights.value)
      }
      // 这里必须重新指向，否则会重复除权
      this.sourceSeer = {
        key: 'SEER',
        fields: FIELD_SEER,
        value: lastval
      }
    }
    // 整理标记点
    this.showSeer = this.filterSeer() // name,date,[该按钮对应的id列表],chart按钮
    // 创建标记点
    for (const k in this.showSeer) {
      this.showSeer[k].chart = this.father.createButton(this.showSeer[k].name)
      maxmin.max = maxmin.max < this.showSeer[k].date ? this.showSeer[k].date : maxmin.max
      maxmin.min = maxmin.min > this.showSeer[k].date ? this.showSeer[k].date : maxmin.min
    }
    // ???
    // 下面开始压缩数据
    // let out = copyArrayOfDeep(this.data.value)

    this.hotKey = 'SEERDAY'
    this.data = { key: 'SEERDAY', fields: FIELD_DATE, value: this.data.value }

    this.linkInfo.showMode = 'fixed'
    // this.linkInfo.fixed.left = 20
    // this.linkInfo.fixed.right = 20
    this.linkInfo.fixed.min = maxmin.min
    this.linkInfo.fixed.max = maxmin.max
  }
  /**
   * draw transfer react
   * @param {Number} left
   * @param {Number} right
   * @memberof ClDrawSeer
   */
  drawTransRect (left, right) {
    if (right < left) return
    const clr = _setTransColor(this.color.grid, 0.5)
    _fillRect(this.context, left, this.rectMain.top, right - left, this.rectMain.height, clr)
  }
  /**
   * paint seer chart
   * @param {Number} key
   * @memberof ClDrawSeer
   */
  onPaint (key) {
    // if (key !== undefined) this.hotKey = key
    // this.data = this.source.getData(this.hotKey)
    // 设置可见
    for (const k in this.showSeer) {
      let showPrice
      if (this.showSeer[k].uids.length < 1) continue
      // 如果只有一条记录就以买入价为定位，否则以收盘价为定位
      if (this.showSeer[k].uids.length === 1) {
        showPrice = getValue(this.sourceSeer, 'buy', this.showSeer[k].nos[0])
      }
      if (this.showSeer[k].focused === true) {
        this.father.setHotButton(this.showSeer[k].chart)
        if (this.hotSeer.value.length === 1) {
          showPrice = getValue(this.sourceSeer, 'buy', this.showSeer[k].hotIdx)
        }
      }
      const seerPos = this.getSeerPos(k, showPrice)

      let num = '*'
      if (this.showSeer[k].uids.length < 10) {
        num = this.showSeer[k].uids.length.toString()
      }
      const acrR = this.layout.symbol.size / 2

      this.showSeer[k].chart.init({
        rectMain: {
          left: seerPos.xx - acrR,
          top: this.showSeer[k].focused ? seerPos.yy - acrR - 2 * acrR : seerPos.yy - acrR,
          width: 2 * acrR,
          height: this.showSeer[k].focused ? 2 * acrR + 2 * acrR : 2 * acrR
        },
        config: {
          translucent: true,
          visible: seerPos.visible,
          status: this.showSeer[k].focused ? 'focused' : 'enabled',
          shape: 'set'
        },
        info: [{
          map: num
        }]
      },
      result => {
        // const self = result.self.father
        const hotInfo = intersectArray(this.showSeer[k].uids, this.hotSeer.value)
        if (hotInfo.length > 0) {
          this.hotSeer.value = []
          this.father.callback({
            event: 'seerclick',
            data: []
          })
          this.source.onPaint()
        } else {
          this.hotSeer.value = this.showSeer[k].uids
          this.father.callback({
            event: 'seerclick',
            data: this.showSeer[k].uids
          })
          this.source.onPaint()
        }
      })
    }
    if (this.hotSeer.value.length === 1) {
      for (let k = 0; k < this.sourceSeer.value.length; k++) {
        if (getValue(this.sourceSeer, 'uid', k) === this.hotSeer.value[0]) {
          this.drawHotSeer(k)
        }
      }
    }
  }
}
