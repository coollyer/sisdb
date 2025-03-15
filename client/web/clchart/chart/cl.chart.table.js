'use strict'

// ////////////////////////////////////////////////////////////////
//   快速表格定义
// ///////////////////////////////////////////////////////////////
import {
    _fillRect,
    _drawRect,
    _setLineWidth,
    _getTxtWidth,
    _drawTxt,
    _drawHline,
    _drawVline,
    _drawBegin,
    _drawEnd
} from './cl.draw'
import {
    CHART_LAYOUT,
    CHART_TABLE
} from '../cl.chart.def'
import {
    fromTTimeToStr,
    formatVolume,
    formatPrice,
    formatFloat,
    formatInteger,
    inArray,
    copyJsonOfDeep,
    updateJsonOfDeep,
    offsetRect
  } from '../cl.utils'
import {
    initCommonInfo,
    checkLayout
} from './cl.chart.init'

import getValue, {
    sortNumberArray,
    sortStringArray,
    changeSortMode
} from '../datas/cl.data.tools'

import ClDrawCursor from './cl.draw.cursor'

export default class ClChartTable {
    /**
  
     * Creates an instance of ClChartLine.
     * @param {Object} father line chart's parent context
     */
    constructor (father) {
      initCommonInfo(this, father)
      
      this.dataLayer = father.dataLayer
      this.linkInfo = this.dataLayer.linkInfo
    //   console.log('this.linkInfo', this.linkInfo.curKey);
    //   this.keyInfo = this.father.dataLayer.keyInfo
    //   this.data = {}
    }
    init (cfg, callback) {
        this.callback = callback
        this.rectMain = cfg.rectMain || { left: 0, top: 0, width: 200, height: 300 }
        this.layout = updateJsonOfDeep(cfg.layout, CHART_LAYOUT)
    
        this.config = updateJsonOfDeep(cfg.config, CHART_TABLE)
        
        this.tableInfo = cfg.tableInfo
        // 这里补充信息
        for (let i = 0; i < this.tableInfo.length; i++) {
            if (this.tableInfo[i].getvar === undefined) {
                this.tableInfo[i].getvar = 'getFltVar'
            }
            if (this.tableInfo[i].getclr === undefined) {
                this.tableInfo[i].getclr = 'getFltClr'
            }
            if (this.tableInfo[i].align === undefined) {
                this.tableInfo[i].align = 'right'
            }
            if (this.tableInfo[i].sort === undefined) {
                this.tableInfo[i].sort = 'sort'
            }
        }
        // 下面对配置做一定的校验
        this.checkConfig()
        // 再做一些初始化运算，下面的运算范围是初始化设置后基本不变的数据
        this.setPublicRect()
        // 初始化子窗口
        this.childDraws = {}
        this.setChildDraw()
        
        this.drawInfo = {
            beginFieldsIdx : 0,  // 当前字段左移索引数
            mayMoveFields : this.tableInfo.length - this.config.fieldlock,
            beginIdx : 0,  // 当前页面开始记录
            selectIndex : -1,  // 当前选中的记录索引 需要根据初始化信息来定位
            cursorIndex : -1, // 鼠标所在的记录索引 需要根据数据来定位
            pageRecs  : -1,    // 每页记录数
            pageCount : -1,    // 全部页数
            pageIndex : -1,    // 当前页索引
        }
        this.readyDraw()
        console.log(this.tableInfo, this.drawInfo);
      }
      checkConfig () { // 检查配置有冲突的修正过来
        checkLayout(this.layout)

        this.config.header.pixel *= this.scale;
        this.config.header.spaceX *= this.scale;
        this.config.header.spaceY *= this.scale;
        this.config.record.pixel *= this.scale;
        this.config.record.spaceX *= this.scale;
        this.config.record.spaceY *= this.scale;
        if (this.config.header.spaceX < this.config.record.spaceX) {
            this.config.header.spaceX = this.config.record.spaceX
        }
        this.numLen = _getTxtWidth(this.context, '8', this.config.record.font, this.config.record.pixel)
        
        this.config.header.height = this.config.header.pixel + 2 * this.config.header.spaceY + this.scale;
        this.config.record.height = this.config.record.pixel + 2 * this.config.record.spaceY + this.scale;
        
      }
      setPublicRect () {
        this.rectChart = offsetRect(this.rectMain, this.layout.margin)
        // 计算title和mess矩形位置
        this.rectTitle = {
            left: 0,
            top:  0,
            width: 0,
            height: 0
        }
        this.rectMess = {
            left: this.rectChart.left,
            top: this.rectChart.top,
            width: 0,
            height: 0
        }
        if (this.config.header.display !== 'none') {
            this.rectTitle = {
                left: this.rectChart.left,
                top: this.rectChart.top,
                width: this.rectChart.width,
                height: this.config.header.height
            }
            this.rectMess = {
                left: this.rectChart.left,
                top: this.rectChart.top + this.config.header.height,
                width: this.rectChart.width,
                height: this.rectChart.height - this.config.header.height,
            }
        }
        else {
            this.rectTitle = {
                left: 0,
                top: 0,
                width: 0,
                height: 0
            }
            this.rectMess = {
                left: this.rectChart.left,
                top: this.rectChart.top,
                width: this.rectChart.width,
                height: this.rectChart.height,
            }
        }
      }
      setChildDraw () {
        this.childDraws['CURSOR'] = new ClDrawCursor(this, { rectMain: this.rectMess, rectChart: this.rectChart, format : 'horline' })
      }
      moveFields(moveIndex) {
        // console.log(moveIndex, this.drawInfo.beginFieldsIdx, this.drawInfo.mayMoveFields);
        if (moveIndex < 0) {
            if (this.drawInfo.beginFieldsIdx > 0) {
                this.drawInfo.beginFieldsIdx += moveIndex
                if (this.drawInfo.beginFieldsIdx < 0) {
                    this.drawInfo.beginFieldsIdx = 0
                }
            }
        }
        if (moveIndex > 0) {
            if (this.drawInfo.beginFieldsIdx < this.drawInfo.mayMoveFields - 1) {
                this.drawInfo.beginFieldsIdx += moveIndex
                if (this.drawInfo.beginFieldsIdx > this.drawInfo.mayMoveFields - 1) {
                    this.drawInfo.beginFieldsIdx = this.drawInfo.mayMoveFields - 1
                }
            }
        }
        // console.log(moveIndex, this.drawInfo.beginFieldsIdx);
        this.readyDraw()
        this.onPaint()
      }
      moveRecord(moveIndex) {
        if (moveIndex < 0) {
            if (this.drawInfo.beginIdx > 0) {
                this.drawInfo.beginIdx--
            }
        }
        if (moveIndex > 0) {
            if (this.drawInfo.beginIdx < this.drawInfo.maxBeginIdx) {
                this.drawInfo.beginIdx++
            }
        }
        this.readyDraw()
        this.onPaint()
      }
      onKeyDown(event) {
        // console.log(event)
        switch (event.keyCode) {
            case 38: // up
                this.moveRecord(-1)
                break
            case 40: // down
                this.moveRecord(1)
                break
            case 37: // left
                this.moveFields(-1)
                break
            case 39: // right
                this.moveFields(1)
                break
        }
      }
      onMouseOut (event) {
        this.childDraws['CURSOR'].onClear()
      }
      getMouseXIndex(mousePos) {
        if (mousePos.y <= this.rectMess.top) {
            return 0
        } 
        else {
            let yy = mousePos.y - this.rectMess.top
            let index = Math.floor(yy / this.config.record.height)
            if (index > this.drawInfo.pageRecs) {
                index = this.drawInfo.pageRecs
            }
            if (index > this.drawInfo.maxDataNums) {
                index = this.drawInfo.maxDataNums
            }
            return index
        }
      }
      getMouseInFieldIndex(mousePos) {
        
        if ((mousePos.x < this.rectTitle.left) || (mousePos.x > this.rectTitle.left + this.rectTitle.width) ||
            (mousePos.y < this.rectTitle.top) || (mousePos.y > this.rectTitle.top + this.rectTitle.height)) {
            return -1
        } 
        else {
            let moveX = this.rectTitle.left;
            let movei = this.drawInfo.beginFieldsIdx
            for (let i = 0; i < this.tableInfo.length; i++) {
                if (movei > 0) {
                    if (i >= this.config.fieldlock) {
                        movei--
                        continue
                    }
                }
                const stepX = this.tableInfo[i].width + 2 * this.drawInfo.spaceX;
                if (mousePos.x > (moveX) && mousePos.x < (moveX + stepX)) {
                    return i
                }
                moveX +=  stepX + this.scale;
                if (moveX > this.rectTitle.left + this.rectTitle.width) {
                    break
                }
            }
            return -1
        }
      }
      onMouseMove (event) {
        // 找到X坐标对应的数据索引
        this.drawInfo.mousePos = event.mousePos
        const valueY = this.rectMess.top + this.config.record.height * (this.getMouseXIndex(this.drawInfo.mousePos) + 1)
        
        this.childDraws['CURSOR'].onPaint(this.drawInfo.mousePos, 0, valueY)
      }
      onClick (event) {
        this.drawInfo.mousePos = event.mousePos
        const fielsIdx = this.getMouseInFieldIndex(this.drawInfo.mousePos)
        
        if (fielsIdx >= 0) {
            if (this.drawInfo.sortFieldIdx === fielsIdx) {
                this.drawInfo.sortFieldMode = changeSortMode(this.drawInfo.sortFieldMode)
            } else {
                this.drawInfo.sortFieldIdx = fielsIdx
                this.drawInfo.sortFieldMode = 'descending'
            }
            this.onPaint()
        } else {
            const newIndex = this.getMouseXIndex(this.drawInfo.mousePos) + this.drawInfo.beginIdx
            // console.log(newIndex);
            if (newIndex !== this.drawInfo.selectIndex) {
                this.drawSelected(this.drawInfo.selectIndex, false)
                this.drawInfo.selectIndex = newIndex
                this.drawSelected(this.drawInfo.selectIndex, true)
            }
        }
        // console.log(event, this.drawInfo.selectIndex, this.drawInfo.beginIdx);
        // this.onPaint()
      }
      onDbClick(event) {
        // console.log(event);
        // const curInfo = this.drawInfo.showInfo[this.drawInfo.selectIndex - this.drawInfo.beginIdx]
        // if (this.drawInfo.selectIndex < 0) {
        //     this.onClick(event)
        // }
        if (this.drawInfo.selectIndex >= 0) {
        
            this.callback({
            event: 'keydown',
            key : 'Enter',
            code : 'Enter',
            keyCode : 13,
            curIndex: this.drawInfo.selectIndex
          })
        }
        // 向上级传递 Enter 信息
      }
      onMouseWheel (event) {
        // console.log('onMouseWheel', event)
        if (Math.abs(event.deltaX) > Math.abs(event.deltaY)) {
            if (event.deltaX !== 0) {
                let moveI = Math.abs(event.deltaX) >= 10 ? event.deltaX / 10 : (event.deltaX > 0 ? 1 : -1)
                this.moveFields(moveI)
            }
        } 
        else {
            if (event.deltaY !== 0) {
                let moveI = Math.abs(event.deltaY) >= 10 ? event.deltaY / 10 : (event.deltaY > 0 ? 1 : -1)
                this.moveRecord(moveI)
            }
        }
    }
    drawClear () {
        _fillRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back)
        _drawBegin(this.context, this.color.grid)
        _drawRect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)
        _drawEnd(this.context)
    }
    drawGrid () {
        _drawBegin(this.context, this.color.grid);
        _fillRect(this.context, this.rectTitle.left, this.rectTitle.top, this.rectTitle.width, this.rectTitle.height, this.color.header)
        _drawHline(this.context, this.rectTitle.left, this.rectTitle.left + this.rectTitle.width, this.rectTitle.top + this.config.header.height - this.scale)
        
        let moveX = this.rectTitle.left + this.drawInfo.spaceX;
        let movei = this.drawInfo.beginFieldsIdx
        for (let i = 0; i < this.tableInfo.length; i++) {
            if (movei > 0) {
                if (i >= this.config.fieldlock) {
                    movei--
                    continue
                }
            }
            moveX += this.tableInfo[i].width + this.drawInfo.spaceX;
            _drawVline(this.context, moveX + this.scale, this.rectMain.top, this.rectMain.top + this.rectMain.height)
            moveX +=  this.scale + this.drawInfo.spaceX;
            if (moveX > this.rectMain.left + this.rectMain.width) {
                break
            }
        }
        let moveY = this.rectMess.top;
        for (let k = 0; k < this.drawInfo.pageRecs; k++) {
            moveY += this.config.record.height;
            _drawHline(this.context, this.rectMess.left, this.rectMess.left + this.rectMess.width, moveY - this.scale)
        }
        _drawEnd(this.context);
    }
    drawScroll () {
        // _fillRect(this.context, this.rectChart.left, this.rectChart.top, this.rectChart.width, this.rectChart.height, this.color.back)
    }
    drawTitle () {
        let moveX = this.rectTitle.left + this.drawInfo.spaceX;
        let moveY = this.rectTitle.top + this.config.header.spaceY;
        
        _drawBegin(this.context, this.color.header);
        
        let movei = this.drawInfo.beginFieldsIdx
        for (let i = 0; i < this.tableInfo.length; i++) {
            if (movei > 0) {
                if (i >= this.config.fieldlock) {
                    movei--
                    continue
                }
            }
            let showX = 0;
            const showW = _getTxtWidth(this.context, this.tableInfo[i].title, this.config.header.font, this.config.header.pixel)
            // 这里可以检查长度是否超过 width 如果超出需要缩小字体大小
            if (this.tableInfo[i].align !== 'left') {
                
                if (this.tableInfo[i].align === 'center') {
                    showX += (this.tableInfo[i].width - showW) / 2;
                }
                else {
                    showX += this.tableInfo[i].width - showW;

                }
            }
            let clr = this.color.txt
            if (this.drawInfo.sortFieldIdx === i) {
                if (this.drawInfo.sortFieldMode === 'ascending') {
                    clr = this.color.green
                }
                if (this.drawInfo.sortFieldMode === 'descending') {
                    clr = this.color.red
                }
            }
            
            // console.log('object,', this.tableInfo[i].title, moveX + showX, moveY);
            _drawTxt(this.context, moveX + showX, moveY, this.tableInfo[i].title,
                    this.config.header.font, this.config.header.pixel, clr);
            moveX += this.tableInfo[i].width + this.drawInfo.spaceX + this.scale + this.drawInfo.spaceX;
            if (this.drawInfo.stopFieldsIdx >= 0 && i >= this.drawInfo.stopFieldsIdx) {
                break
            }
        }
        _drawEnd(this.context);
    
    }
    drawRecord (moveX, moveY, ridx, fidx) {
        let showX = 0;
        const curInfo = this.drawInfo.showInfo[ridx - this.drawInfo.beginIdx][fidx]
        // console.log(curInfo);
        const showW = _getTxtWidth(this.context, curInfo.text, this.config.record.font, this.config.record.pixel)
        // 这里可以检查长度是否超过 width 如果超出需要缩小字体大小
        if (this.tableInfo[curInfo.fidx].align !== 'left') {
            
            if (this.tableInfo[curInfo.fidx].align === 'center') {
                showX += (this.tableInfo[curInfo.fidx].width - showW) / 2;
            }
            else {
                showX += this.tableInfo[curInfo.fidx].width - showW;

            }
        }
        // console.log('object,', i, k, this.drawInfo.showInfo[k]);
        _drawTxt(this.context, moveX + showX, moveY + this.config.record.spaceY, curInfo.text,
                    this.config.record.font, this.config.record.pixel, curInfo.color);
        
    }
    drawSelected (ridx, selected) {
        if (ridx < 0) {
            return
        }
        const curidx = ridx - this.drawInfo.beginIdx
        if (curidx < 0 || curidx > this.drawInfo.showInfo.length - 1) {
            return
        }
        const moveY = this.rectMess.top + this.config.record.height * (ridx - this.drawInfo.beginIdx)
        // console.log('====', ridx, this.drawInfo.selectIndex, this.drawInfo.showInfo[ridx - this.drawInfo.beginIdx]);
        _fillRect(this.context, this.rectMess.left, moveY, this.rectMess.width, this.config.record.height - this.scale, selected ? this.color.select : this.color.back)
        _drawBegin(this.context, this.color.grid);
        let moveX = this.rectMess.left + this.drawInfo.spaceX;
        for (let fidx = 0; fidx < this.drawInfo.showInfo[ridx - this.drawInfo.beginIdx].length; fidx++) {
            this.drawRecord(moveX, moveY, ridx, fidx)
            _drawVline(this.context, moveX - this.drawInfo.spaceX, moveY, moveY + this.config.record.height)
            moveX += this.tableInfo[fidx].width + this.drawInfo.spaceX + this.scale + this.drawInfo.spaceX;
        }
        _drawEnd(this.context);
    }
    drawTable () {
        // console.log('====', this.drawInfo.showInfo);
        
        _drawBegin(this.context, this.color.grid);
        let moveY = this.rectMess.top;
        for (let ridx = this.drawInfo.beginIdx; ridx < this.drawInfo.beginIdx + this.drawInfo.pageRecs && ridx < this.drawInfo.maxDataNums; ridx++) {
            let moveX = this.rectMess.left + this.drawInfo.spaceX;
            if (ridx === this.drawInfo.selectIndex) {
                this.drawSelected(ridx, true)
            }
            else {
                for (let fidx = 0; fidx < this.drawInfo.showInfo[ridx - this.drawInfo.beginIdx].length; fidx++) {
                    this.drawRecord(moveX, moveY, ridx, fidx)
                    moveX += this.tableInfo[fidx].width + this.drawInfo.spaceX + this.scale + this.drawInfo.spaceX;
                }
            }
            moveY += this.config.record.height;
        }
        _drawEnd(this.context);
    }
    onPaint () {
        _setLineWidth(this.context, this.scale)
        this.drawClear() // 清理画图区
        
        this.readyData()

        this.drawGrid() // 画线格
        
        // this.drawScroll() // 画滚动块    
        
        this.drawTitle() // 画标题  
        this.drawTable() // 画表格  
        
        
        // this.drawGrid() // 画线格
        // this.img = _getImageData(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height)
      }
      readyDraw () { 
        
        // 计算长宽信息
        let curww = 0;
        const font = this.config.header.font > this.config.record.font ? this.config.header.font : this.config.record.font;
        const pixel = this.config.header.pixel > this.config.record.pixel ? this.config.header.pixel : this.config.record.pixel;
        
        this.drawInfo.stopFieldsIdx = -1
        const minww = this.config.maxlen * this.numLen;
        let movei = this.drawInfo.beginFieldsIdx
        let useFields = 0
        for (let i = 0; i < this.tableInfo.length; i++) {
            // console.log(this.tableInfo[i].len);
            if (movei > 0) {
                if (i >= this.config.fieldlock) {
                    movei--
                    continue
                }
            }
            if (this.tableInfo[i].len === undefined) {
                this.tableInfo[i].width = _getTxtWidth(this.context, this.tableInfo[i].title, font, pixel);
                this.tableInfo[i].width = this.tableInfo[i].width > minww ? this.tableInfo[i].width : minww;
            } else {
                this.tableInfo[i].width = this.tableInfo[i].len * this.numLen;
            }
            if (curww + this.tableInfo[i].width + 2 * this.config.header.spaceX > this.rectMain.width) {
                this.drawInfo.stopFieldsIdx = i
                break
            }
            useFields++
            curww += this.tableInfo[i].width + 2 * this.config.header.spaceX + this.scale;
        }
        // console.log(this.drawInfo.spaceX, curww, this.drawInfo.stopFieldsIdx);
        if (this.drawInfo.stopFieldsIdx >= 0) {
            this.drawInfo.spaceX = this.config.header.spaceX + (this.rectMess.width - curww) / (2 * (useFields + 1))
        }
        else {
            this.drawInfo.spaceX = this.config.header.spaceX;
        } 
        // console.log('--', this.drawInfo.spaceX);       
        this.drawInfo.pageRecs = Math.floor(this.rectMess.height / this.config.record.height);
    }
    getData() {
        this.data = this.father.getData(this.hotKey)
        if (this.data === undefined) return null;
        let curdata = copyJsonOfDeep(this.data)
        // 这里根据排序方式进行数据调整
        if (!(this.drawInfo.sortFieldIdx < 0 || this.drawInfo.sortFieldMode === undefined)) {
            console.log('----', this.drawInfo);
            if (this.tableInfo[this.drawInfo.sortFieldIdx].sort === 'resume' || this.drawInfo.sortFieldMode === 'preface') {
                this.drawInfo.sortFieldIdx = -1
                this.drawInfo.sortFieldMode = 'preface'
            }
            else {
                if (this.tableInfo[this.drawInfo.sortFieldIdx].sort === 'string') {
                    sortStringArray(curdata.value, this.drawInfo.sortFieldIdx, this.drawInfo.sortFieldMode)                   
                } 
                else {
                    sortNumberArray(curdata.value, this.drawInfo.sortFieldIdx, this.drawInfo.sortFieldMode)
                }
            }
        }
        return curdata
    }
    initSelectIndex(curdata) {
        if (this.linkInfo.curKey === undefined || this.config.curKeyIdx === undefined) {
            return 
        }
        for (let index = 0; index < this.drawInfo.showInfo.length; index++) {
            const curv = this.drawInfo.showInfo[index][this.config.curKeyIdx]
            // console.log(curv, curdata.value[curv.ridx][this.config.curKeyIdx]) //, curv[this.config.curKeyIdx].index, curdata.value[curv.index], this.config, this.linkInfo);
            if (curdata.value[curv.ridx][this.config.curKeyIdx] === this.linkInfo.curKey) {
                this.drawInfo.selectIndex = curv.ridx
            }
        }
        // console.log(this.drawInfo.selectIndex)
    }
    readyData () { 
        // 计算最大最小值等
        const curdata = this.getData()
        if (!curdata) {
            return ;
        }
        if (this.keyInfo === undefined) {
            this.keyInfo = this.father.dataLayer.keyInfo
        }
        // console.log(curdata.value);
        // 下面计算基础的参数
        this.drawInfo.maxDataNums = curdata.value.length;
        this.drawInfo.pageCount = this.drawInfo.maxDataNums / this.drawInfo.pageRecs;
        this.drawInfo.pageIndex = 0
        this.drawInfo.maxBeginIdx = this.drawInfo.maxDataNums - this.drawInfo.pageRecs
        if (this.drawInfo.maxBeginIdx < 0) {
            this.drawInfo.maxBeginIdx = 0
        }
        // 这里要根据鼠标定位 cursorIndex 
        // 这里要根据默认代码定位 selectIndex 以确定开始的页数 pageIndex 然后可以确定 beginIdx
        // console.log(this.linkInfo)

        // 这里开始提取显示内容
        this.drawInfo.showInfo = [];
        
        let clr, val, txt;
        for (let k = this.drawInfo.beginIdx; k < this.drawInfo.beginIdx + this.drawInfo.pageRecs && k < this.drawInfo.maxDataNums; k++) {
            // console.log('object', k, yy, this.beginIdx, this.rectMess.top, this.rectMess.height);
            const curKey = getValue(curdata, this.tableInfo[this.config.curKeyIdx].label, k)
            let showstr = [];
            let movei = this.drawInfo.beginFieldsIdx
            for (let i = 0; i < this.tableInfo.length; i++) {
                if (movei > 0) {
                    if (i >= this.config.fieldlock) {
                        movei--
                        continue
                    }
                }
                if (this.drawInfo.stopFieldsIdx >= 0  && i > this.drawInfo.stopFieldsIdx) {
                    break
                }
            // for (let i = 0; i < this.tableInfo.length; i++) {                
                val = getValue(curdata, this.tableInfo[i].label, k);
                
                switch (this.tableInfo[i].getvar) {
                    case "getVolVar":
                        txt = formatVolume(val, 1);
                        break;
                    case "getIntVar":
                        txt = formatInteger(val, 1);
                        break;
                    case "getPrcVar":
                        txt = formatPrice(val, 2);
                        break;
                    case "getFltVar":
                        txt = formatFloat(val, 3, this.config.maxlen);
                        break;
                    case "getPctVar":
                        txt = formatPrice(val * 100, 2) + '%';
                        break;
                    case "getStrVar":
                    default:
                        txt = val.toString();
                        break;
                }
                switch (this.tableInfo[i].getclr) {
                    case "getCodeClr":
                        clr = this.color.code;
                        break;
                    case "getNameClr":
                        clr = this.color.name;
                        break;
                    case "getIntClr":
                        clr = this.color.yellow;
                        break;
                    case "getPrcClr":
                        // 需要和前收盘做比较
                        // console.log(this.keyInfo, curKey);
                        const agop = this.keyInfo[curKey].static.agoprc
                        clr = val > agop ? this.color.red : (val < agop ? this.color.green : this.color.yellow)
                        break;
                    case "getFltClr":
                        clr = this.color.yellow;
                        break;
                    case "getZeroClr":
                        clr = val > 0.0000001 ? this.color.red : (val < -0.0000001 ? this.color.green : this.color.white)
                        break;
                    case "getVolClr":
                        clr = this.color.vol;
                        break;
                    default:
                        clr = this.color.yellow;
                }
                showstr.push({ ridx: k, fidx: i, text: txt, color: clr });
            }
            this.drawInfo.showInfo.push(showstr);
        }
        // console.log(this.drawInfo.selectIndex, this.linkInfo.curKey);
        if (this.drawInfo.selectIndex === -1) {
            this.initSelectIndex(curdata)
        }
      }
}

// function ClChartTable() {
//     // ////////////////////////////////////////////////////////////////
//     //   程序入口程序，以下都是属于设置类函数，基本不需要修改，
//     // ///////////////////////////////////////////////////////////////
//     this.init = function(usercfg, callback) {
//       this.scale = this.baba.scale;
//       this.rectMain = usercfg.rectMain || {
//         left: 0,
//         top: 0,
//         width: 800,
//         height: 600
//       };
//       this.setcfg(usercfg);
//       this.callback = callback;
//     }
//     this.loaddefaultconfig = function() { // 加载默认配置文件
//       this.scrollHor = {
//         width: 20,
//         display: false
//       };
//       this.scrollVer = {
//         width: 20,
//         display: true
//       };
//       this.leftlock = 5; // 按左键时，前面5个字段不动
//       this.tableInfo = [{
//         txt: '代码',
//         label: 'code',
//         len: 6,
//         alignTxt: 'left',
//         sort: 'resume'
//       }, // resume表示原来的序列
//       {
//         txt: '名称',
//         label: 'name',
//         len: 8,
//         alignTxt: 'left',
//         sort: 'resume'
//       }, // align 默认为'center'
//       {
//         txt: '最新价',
//         label: 'close',
//         change: true,
//         sort: 'sort'
//       }, // alignTxt默认为'right'  sort默认为‘sort’
//       {
//         txt: '涨跌',
//         label: 'change'
//       },
//       {
//         txt: '涨幅%',
//         label: 'increase'
//       },
//       {
//         txt: '现量',
//         label: 'nowvol',
//         change: true
//       }, // change:true表示如果数值有变化，显示动画效果
//       {
//         txt: '成交量',
//         label: 'vol'
//       },
//       {
//         txt: '成交额',
//         label: 'money'
//       },
//       {
//         txt: '昨收',
//         label: 'preclose'
//       },
//       {
//         txt: '今开',
//         label: 'open'
//       },
//       {
//         txt: '最高',
//         label: 'high'
//       },
//       {
//         txt: '最低',
//         label: 'low'
//       },
//       {
//         txt: '振幅%',
//         label: 'swing'
//       },
//       {
//         txt: '涨停',
//         label: 'stophigh'
//       },
//       {
//         txt: '跌停',
//         label: 'stoplow'
//       },
//       {
//         txt: '买一',
//         label: 'buy1'
//       },
//       {
//         txt: '卖一',
//         label: 'sell1'
//       }
//       ];
//     }
//     this.setcfg = function(cfg) {
//       this.config = {};
//       this.loaddefaultconfig();
//       this.scrollHor = cfg.scrollHor || this.scrollHor;
//       this.scrollVer = cfg.scrollVer || this.scrollVer;
//       this.leftlock = cfg.leftlock || this.leftlock;
//       this.config = {
//         margin: {
//           left: cfg.margin ? cfg.margin.left || 0 : 0,
//           top: cfg.margin ? cfg.margin.top || 0 : 0,
//           right: cfg.margin ? cfg.margin.right || 0 : 0,
//           bottom: cfg.margin ? cfg.margin.bottom || 0 : 0
//         }, // 边界偏移值
//         title: {
//           display: cfg.title ? cfg.title.display || 'true' : 'true',
//           align: cfg.title ? cfg.title.align || 'center' : 'center',
//           height: cfg.title ? cfg.title.height || 20 : 20,
//           spaceX: cfg.title ? cfg.title.spaceX || 8 : 8,
//           pixel: cfg.title ? cfg.title.pixel || 12 : 12,
//           font: cfg.title ? cfg.title.font || 'sans-serif' : 'sans-serif'
//         },
//         row: {
//           showmode: cfg.title ? cfg.title.showmode || 'none' : 'none', // none||color||line //行设置
//           onecolor: cfg.title ? cfg.title.onecolor || '#eee' : 'eee',
//           twocolor: cfg.title ? cfg.title.twocolor || '#ccc' : '#ccc',
//           height: cfg.title ? cfg.title.height || 26 : 26,
//           spaceX: cfg.title ? cfg.title.spaceX || 8 : 8,
//           pixel: cfg.title ? cfg.title.pixel || 16 : 16,
//           font: cfg.title ? cfg.title.font || 'sans-serif' : 'sans-serif'
//         },
//         background: cfg.background || 'black'
//       }
//       this.setbackground(this.config.background);
//       // 下面对配置做一定的校验
//       this.checkconfig();
//       // 再做一些初始化运算，下面的运算范围是初始化设置后基本不变的数据
//       this.setpublicrect();
//       // console.log(this.config);
//     }
//     this.checkconfig = function() { // 检查配置有冲突的修正过来
//       this.config.margin.top *= this.scale;
//       this.config.margin.left *= this.scale;
//       this.config.margin.bottom *= this.scale;
//       this.config.margin.right *= this.scale;
  
//       this.config.header.pixel *= this.scale;
//       this.config.header.height *= this.scale;
//       this.config.header.spaceX *= this.scale;
  
//       this.config.row.pixel *= this.scale;
//       this.config.row.height *= this.scale;
//       this.config.row.spaceX *= this.scale;
  
//       if (this.config.header.height < (this.config.header.pixel + 2 * this.scale)) {
//         this.config.header.height = this.config.header.pixel + 2 * this.scale;
//       }
//     }
//     this.setbackground = function(color) { // 设置背景颜色方案
//       this.config.background = color;
//       if (this.config.background === 'white') {
//         this.color = _chart_whitecolor;
//       } else {
//         this.color = _chart_blackcolor;
//       }
//     }
//     this.setpublicrect = function() { // 计算所有矩形区域
//       const xx = this.rectMain.left + this.config.margin.left;
//       const yy = this.rectMain.top + this.config.margin.top;
//       const ww = this.rectMain.width - (this.config.margin.left + this.config.margin.right);
//       const hh = this.rectMain.height - (this.config.margin.top + this.config.margin.bottom);
  
  
//       if (this.config.header.display !== 'none') {
//         this.rectTitle = {
//           left: xx,
//           top: yy,
//           width: ww,
//           height: this.config.header.height
//         };
//         this.rectChart = {
//           left: xx,
//           top: yy + this.config.header.height + this.scale,
//           width: ww,
//           height: hh - this.config.header.height - 2 * this.scale
//         };
//       } else {
//         this.rectTitle = {
//           left: 0,
//           top: 0,
//           width: 0,
//           height: 0
//         };
//         this.rectChart = {
//           left: xx,
//           top: yy,
//           width: ww,
//           height: hh
//         };
//       }
//       // 这里就直接一次性计算完所有的参数
//       this.init_table_info();
//     }
//   this.init_table_info = function() {
//     // 当改变了头信息，可以调用这里重新计算

//     let ww;
//     const font = this.config.header.font > this.config.row.font ? this.config.header.font : this.config.row.font;
//     const pixel = this.config.header.pixel > this.config.row.pixel ? this.config.header.pixel : this.config.row.pixel;
//     this.sign_width = _gettxtwidth(this.context, '8', font, pixel);
//     // console.log('this.sign_width',this.sign_width);
//     ww = this.config.header.spaceX;
//     const minww = 7 * this.sign_width;
//     for (let i = 0; i < this.tableInfo.length; i++) {
//       // console.log(this.tableInfo[i].len);
//       if (this.tableInfo[i].len === undefined) {
//         this.tableInfo[i].width = _gettxtwidth(this.context, this.tableInfo[i].txt, font, pixel);
//         this.tableInfo[i].width = this.tableInfo[i].width > minww ? this.tableInfo[i].width : minww;
//       } else {
//         this.tableInfo[i].width = this.tableInfo[i].len * this.sign_width;
//       }
//       ww += this.sign_width + this.tableInfo[i].width + this.config.header.spaceX;
//     }
//     if (ww > this.rectChart.width) this.spaceX = this.config.header.spaceX;
//     else this.spaceX = (this.rectChart.width - ww) / this.tableInfo.length;

//     this.beginIdx = 0;
//     this.pageCount = Math.floor(this.rectChart.height / this.config.row.height);
//     this.beginFieldsIdx = 0;
//   }
//   this.onPaint = function() { // 重画
//     _setLineWidth(this.context, this.scale);
//     this.draw_clear(); // 清理画图区
//     this.draw_column(); // 先画线格
//     // 为画图做准备工作,计算各种高度和动态宽度
//     if (this.draw_ready()) {
//       this.draw_table(); // 画内容
//     }

//     this.img = _getImageData(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height);
//   }

//   this.draw_ready = function() { // 计算最大最小值等
//     this.data = this.baba.getData(this.datakey);
//     if (this.data === undefined) return false;
//     // 下面计算scroll的参数

//     // INDEX在数据层存储改为CODE
//     this.codeinfo = this.baba.getlinkdata(this.data.value, 'CODE');
//     // console.log(this.codeinfo,'this.codeinfo');
//     this.mapinfo = this.baba.getlinkdata(this.data.value, 'NOW');
//     // 这里应该把所有数据取出来，保存在列表中，然后显示的时候直接显示，新来行情也可以直接比较文本来显示动态效果
//     this.drawInfo.showInfo = [];
//     let clr, txt;
//     let value, save_str;
//     let decimal, pre_close;

//     for (let k = this.beginIdx; k < this.beginIdx + this.pageCount; k++) {
//       // console.log('object', k, yy, this.beginIdx, this.rectChart.top, this.rectChart.height);
//       save_str = [];
//       decimal = this.getValue(k, 'decimal');
//       pre_close = this.getValue(k, 'preclose');
//       for (let i = 0; i < this.tableInfo.length; i++) {
//         value = this.getValue(k, this.tableInfo[i].label);
//         if (_inArray(this.tableInfo[i].label, ['code', 'name'])) {
//           txt = value.toString();
//           if (this.tableInfo[i].label === 'code') txt = txt.substr(2);
//           clr = this.color.code;
//         } else if (_inArray(this.tableInfo[i].label, ['close', 'open', 'high', 'low', 'preclose', 'stophigh', 'stoplow', 'buy1', 'sell1'])) {
//           txt = cl_format_value(value, decimal);
//           clr = this.getcolor(value, pre_close);
//         } else if (_inArray(this.tableInfo[i].label, ['vol', 'money', 'nowvol'])) {
//           txt = cl_format_value(value, 0);
//           clr = this.color.vol;
//         } else if (_inArray(this.tableInfo[i].label, ['increase'])) {
//           txt = cl_format_value(value, 2);
//           clr = this.color.red;
//           if (value < 0) clr = this.color.green;
//         } else if (_inArray(this.tableInfo[i].label, ['swing'])) {
//           txt = cl_format_value(value, 2);
//           clr = this.color.vol;
//         } else if (_inArray(this.tableInfo[i].label, ['change'])) {
//           txt = cl_format_value(value, 2);
//           clr = this.color.red;
//           if (value < 0) clr = this.color.green;
//         } else {
//           txt = '--';
//           clr = this.color.txt;
//         }
//         save_str.push({ label: this.tableInfo[i].label, text: txt, color: clr });
//       }
//       this.drawInfo.showInfo.push(save_str);
//     }
//     return true;
//   }
//   // ////////////////////////////////////////////////////////////////
//   //   绘图函数
//   // ///////////////////////////////////////////////////////////////
//   this.draw_clear = function() {
//     _fillrect(this.context, this.rectMain.left, this.rectMain.top, this.rectMain.width, this.rectMain.height, this.color.back);
//   }

//   this.draw_column = function() {
//     if (this.config.header.display === 'none') return;

//     _drawBegin(this.context, this.color.grid);
//     _drawHline(this.context, this.rectTitle.left, this.rectTitle.left + this.rectTitle.width,
//       this.rectTitle.top + this.config.header.height);
//     const hh = Math.floor((this.config.header.height - this.config.header.pixel) / 2) - this.scale;
//     let xx = this.rectTitle.left + this.spaceX;
//     let offx;
//     let islock = true;
//     let lock = 0;
//     let bw = 0;
//     this.headinfo = [];
//     for (let i = 0; i < this.tableInfo.length; i++) {
//       //  console.log(lock,this.beginFieldsIdx);
//       if (i >= this.leftlock) islock = false;
//       if (!islock) {
//         lock++;
//         if (lock <= this.beginFieldsIdx) continue;
//       }
//       offx = this.tableInfo[i].width - _gettxtwidth(this.context, this.tableInfo[i].txt,
//         this.config.header.font, this.config.header.pixel);
//       _drawtxt(this.context, xx + Math.floor(offx / 2), this.rectTitle.top + hh, this.tableInfo[i].txt,
//         this.config.header.font, this.config.header.pixel, this.color.txt);
//       _drawVline(this.context, xx + this.sign_width + this.tableInfo[i].width, this.rectTitle.top,
//         this.rectTitle.top + this.rectTitle.height);
//       bw = this.sign_width + this.tableInfo[i].width - this.spaceX;
//       this.headinfo.push({ index: i, rectMain: { left: xx, top: this.rectTitle.top,
//         width: bw, height: this.rectTitle.top + this.rectTitle.height },
//         rectSplit: { left: xx + bw, top: this.rectTitle.top,
//           width: this.spaceX * 2, height: this.rectTitle.top + this.rectTitle.height } });
//       xx += this.sign_width + this.tableInfo[i].width + this.spaceX;
//       // console.log(this.tableInfo[i].width);
//     }
//     _drawEnd(this.context);
//   };
//   this._onclick = function(event) {
//     const mousePosY = event.offsetY * this.scale;
//     if (mousePosY >= this.rectChart.top &&
//         mousePosY < (this.rectChart.top + this.rectChart.height)) {
//       let yy = this.rectChart.top;
//       for (let k = this.beginIdx; k < this.beginIdx + this.pageCount; k++) {
//         if (mousePosY >= yy && mousePosY < (yy + this.config.row.height)) {
//           this.callback(this.getValue(k, 'code'));
//           break;
//         }
//         yy += this.config.row.height;
//       }
//     }
//   }
//   this.change_rows = function(step) {
//     this.beginIdx += step;
//     if (step > 0 && this.beginIdx > this.codeinfo.length - this.pageCount) {
//       this.beginIdx = this.codeinfo.length - this.pageCount;
//     }
//     if (this.beginIdx < 0) this.beginIdx = 0;
//     // console.log('beginidx', this.codeinfo.length, this.pageCount, this.beginIdx);
//     this.onPaint();
//   }
//   this._onmouse_wheel = function(event) {
//     let step = Math.floor(event.deltaY / 3);
//     if (event.deltaY < 0) {
//       step = Math.round(event.deltaY / 3);
//     }
//     console.log('step:', event.deltaY, step);
//     this.change_rows(step);
//   }
//   this._onkeydown = function(event) {
//     switch (event.keyCode) {
//       case 38: // up
//         this.change_rows(-1);
//         break;
//       case 40: // down
//         this.change_rows(1);
//         break;
//       case 37: // left
//         if (this.beginFieldsIdx < this.tableInfo.length - this.leftlock - 2) {
//           this.beginFieldsIdx++;
//           this.onPaint();
//         }
//         break;
//       case 39: // right
//         if (this.beginFieldsIdx > 0) {
//           this.beginFieldsIdx--;
//           this.onPaint();
//         }
//         break;
//     }
//     console.log('??? key:', event.keyCode);
//   }
//   this.draw_table = function() {
//     let xx, yy, offx;
//     const offy = Math.floor((this.config.row.height - this.config.row.pixel) / 2) - this.scale;

//     let islock = true;
//     let lock = 0;

//     yy = this.rectChart.top;
//     _drawBegin(this.context, this.color.grid);
//     for (let k = this.beginIdx; k < this.beginIdx + this.pageCount; k++) {
//       // console.log('object', k, yy, this.beginIdx, this.rectChart.top, this.rectChart.height);
//       xx = this.rectChart.left + this.spaceX;
//       lock = 0;
//       islock = true;
//       for (let i = 0; i < this.tableInfo.length; i++) {
//         if (i >= this.leftlock) islock = false;
//         if (!islock) {
//           lock++;
//           if (lock <= this.beginFieldsIdx) continue;
//         }
//         offx = 0;
//         if (this.tableInfo[i].alignTxt !== 'left') {
//           offx = this.tableInfo[i].width - _gettxtwidth(this.context, this.drawInfo.showInfo[k - this.beginIdx][i].text, this.config.row.font, this.config.row.pixel);
//         }
//         // console.log('object,', i, k, this.drawInfo.showInfo[k]);
//         _drawtxt(this.context, xx + offx, yy + offy, this.drawInfo.showInfo[k - this.beginIdx][i].text,
//                  this.config.row.font, this.config.row.pixel, this.drawInfo.showInfo[k - this.beginIdx][i].color);
//         xx += this.sign_width + this.tableInfo[i].width + this.spaceX;
//       }
//       yy += this.config.row.height;
//       _drawHline(this.context, this.rectChart.left, this.rectChart.left + this.rectChart.width, yy);
//     }
//     _drawEnd(this.context);
//   };
//   this.getcolor = function(close, preclose) {
//     if (close > preclose) {
//       return this.color.red;
//     } else if (close < preclose) {
//       return this.color.green;
//     } else {
//       return this.color.white;
//     }
//   }

//   this.getValue = function(index, label) {
//     if (label === undefined || index < 0) return 0;
//     // if (!this.data||!this.codeinfo||!this.mapinfo)return 0;
//     // if(index>=this.data.length) return 0;
//     // if(index>=this.codeinfo.length) return 0;
//     // if(index>=this.mapinfo.length) return 0;
//     let value = 0;
//     let base;
//     const fields = {
//       code: 0,
//       name: 1,
//       jp: 2,
//       type: 3,
//       decimal: 4,
//       preclose: 5,
//       time: 0,
//       open: 1,
//       high: 2,
//       low: 3,
//       close: 4,
//       vol: 5,
//       money: 6,
//       buy1: 7,
//       buyv1: 8,
//       sell1: 9,
//       sellv1: 10,
//       buy2: 11,
//       buyv2: 12,
//       sell2: 13,
//       sellv2: 14,
//       buy3: 15,
//       buyv3: 16,
//       sell3: 17,
//       sellv3: 18,
//       buy4: 19,
//       buyv4: 20,
//       sell4: 21,
//       sellv4: 22,
//       buy5: 23,
//       buyv5: 24,
//       sell5: 25,
//       sellv5: 26
//     };
//     // console.log(this.codeinfo[index]);
//     if (!this.codeinfo[index]) return value;
//     const prc_unit = cl_power(this.codeinfo[index][4]);

//     switch (label) {
//       case 'idx':
//         value = index;
//         break;
//       case 'code':
//         value = this.codeinfo[index][fields.code];
//         break;
//       case 'name':
//         value = this.codeinfo[index][fields.name];
//         break;
//       case 'type':
//         value = this.codeinfo[index][fields.type];
//         break;
//       case 'decimal':
//         value = this.codeinfo[index][fields.decimal];
//         break;
//       case 'vol_unit':
//         value = this.codeinfo[index][fields.vol_unit];
//         break;
//       case 'preclose':
//         value = this.codeinfo[index][fields.preclose] / prc_unit;
//         break;
//       case 'stophigh':
//         // value = this.codeinfo[index][fields.stophigh] / prc_unit;
//         break;
//       case 'stoplow':
//         // value = this.codeinfo[index][fields.stoplow] / prc_unit;
//         break;
//       case 'time':
//         value = this.mapinfo[index][fields.time];
//         break;
//       case 'open':
//         value = this.mapinfo[index][fields.open] / prc_unit;
//         break;
//       case 'high':
//         value = this.mapinfo[index][fields.high] / prc_unit;
//         break;
//       case 'low':
//         value = this.mapinfo[index][fields.low] / prc_unit;
//         break;
//       case 'close':
//         value = this.mapinfo[index][fields.close] / prc_unit;
//         break;
//       case 'change':
//         if (this.mapinfo[index][fields.close] <= 0) break;
//         value = (this.mapinfo[index][fields.close] - this.codeinfo[index][fields.preclose]) / prc_unit;
//         break;
//       case 'swing':
//         if (this.mapinfo[index][fields.close] <= 0) break;
//         base = this.mapinfo[index][fields.low];
//         value = (this.mapinfo[index][fields.high] - base) / base * 100;
//         break;
//       case 'increase':
//         if (this.mapinfo[index][fields.close] <= 0) break;
//         if (this.codeinfo[index][fields.preclose] <= 0) {
//           base = this.mapinfo[index][fields.open];
//         } else {
//           base = this.codeinfo[index][fields.preclose];
//         }
//         value = (this.mapinfo[index][fields.close] - base) / base * 100;
//         break;
//       case 'nowvol':
//         // value = this.mapinfo[index][fields.low] / prc_unit;
//         break;
//       case 'vol':
//         value = this.mapinfo[index][fields.vol] / 100;
//         break;
//       case 'money':
//         value = this.mapinfo[index][fields.money] / 100;
//         break;
//       case 'buy1':
//         value = this.mapinfo[index][fields.buy1] / prc_unit;
//         break;
//       case 'buy2':
//         value = this.mapinfo[index][fields.buy2] / prc_unit;
//         break;
//       case 'buy3':
//         value = this.mapinfo[index][fields.buy3] / prc_unit;
//         break;
//       case 'buy4':
//         value = this.mapinfo[index][fields.buy4] / prc_unit;
//         break;
//       case 'buy5':
//         value = this.mapinfo[index][fields.buy5] / prc_unit;
//         break;
//       case 'sell1':
//         value = this.mapinfo[index][fields.sell1] / prc_unit;
//         break;
//       case 'sell2':
//         value = this.mapinfo[index][fields.sell2] / prc_unit;
//         break;
//       case 'sell3':
//         value = this.mapinfo[index][fields.sell3] / prc_unit;
//         break;
//       case 'sell4':
//         value = this.mapinfo[index][fields.sell4] / prc_unit;
//         break;
//       case 'sell5':
//         value = this.mapinfo[index][fields.sell5] / prc_unit;
//         break;
//       case 'buyv1':
//         value = this.mapinfo[index][fields.buyv1];
//         break;
//       case 'buyv2':
//         value = this.mapinfo[index][fields.buyv2];
//         break;
//       case 'buyv3':
//         value = this.mapinfo[index][fields.buyv3];
//         break;
//       case 'buyv4':
//         value = this.mapinfo[index][fields.buyv4];
//         break;
//       case 'buyv5':
//         value = this.mapinfo[index][fields.buyv5];
//         break;
//       case 'sellv1':
//         value = this.mapinfo[index][fields.sellv1];
//         break;
//       case 'sellv2':
//         value = this.mapinfo[index][fields.sellv2];
//         break;
//       case 'sellv3':
//         value = this.mapinfo[index][fields.sellv3];
//         break;
//       case 'sellv4':
//         value = this.mapinfo[index][fields.sellv4];
//         break;
//       case 'sellv5':
//         value = this.mapinfo[index][fields.sellv5];
//         break;
//     }
//     return value;
//   }
// }

// export default ClChartTable;
