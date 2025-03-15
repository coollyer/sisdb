
import * as Init from '../init.js';
import { CLAPI } from '../clchart/cl.api.js'
import * as DATAS from '../datas/transdata.js'

// 画分钟线
// 分笔成交和买卖盘信息 做单独的chart 
// 根据其按键的状态返回改变窗口 用于重画数据 每个chart仅仅对应一个数据
// 所有数据看板都直接用 table 来显示
const chartName = 'Minute'
function drawMinute (inData, minunums) {

    console.log('Draw ====> Min Line', inData)
    
    const canvas = Init.getCanvas()
    // 设置为单一窗口 并返回第一个窗口
    // 设置区域中的联动配置 指定名字 根据名字加载不同的联动参数
    const chart = Init.systemInfo.curChart.initChart([chartName])
    // 得到第一个窗口
    // 默认已经设置了交易时间
    // chart.setTradeTimes(CLAPI.DatasDef.STOCK_TRADETIME)
    chart.setlinkInfo('curKey', Init.getcurKey())
    
    const keyInfo = DATAS.AinfoToKeyInfo(inData['info'])
    chart.setKeyInfo(keyInfo)
    // console.log(keyInfo);

    // 设置最新时间
    const curDate = CLAPI.Utils.getMsecDate(inData['info'].datas.at(-1)[0])
    chart.setTradeDate(curDate)
    
    // 设置坐标显示信息
    const axisXinfo = DATAS.IominuToAxisXinfo(inData['minute'], 'msec')
    chart.setAxisXinfo(axisXinfo.timeInfo, axisXinfo.dateInfo)

    Init.setUserInfo(chartName, 'minuteCharts', minunums)

    const minus = DATAS.IominuToMinute(inData['minute'])
    const FIELD_IOMINU = {
        msec:  0, // 按交易时间的分钟索引
        open:  1,
        maxp:  2,
        minp:  3,
        newp:  4,
        sumv:  5,
        summ:  6,
        curv:  7,
        curm:  8,
        curbigm: 9,
        sumbigm: 10,
        index: 11,
    }

    // console.log(minus);
    chart.setData('MINUTE', FIELD_IOMINU, minus)
    // 设置行情明细
    // chart.setData('SNAPS', CLAPI.DatasDef.FIELD_SNAP, inData['snapshot'].datas)

    // 设置界面信息和绑定数据
    // 设置画布尺寸
    const mainHeight = canvas.height * CLAPI.CH1
    // let mainWidth = Math.max(canvas.width * 0.65, canvas.width - 400)
    const mainWidth = canvas.width
    
    // 设置画布区域布局
    const mainLayoutCfg = {
        layout: CLAPI.ChartDef.CHART_LAYOUT,
        config: CLAPI.Utils.mergeJsonOfDeep({ 
            axisX: { lines: getAxisXlines(minunums), },
        }, 
        CLAPI.ChartDef.CHART_MINUTE),
        rectMain: {
            left: 0,
            top: 0,
            width: mainWidth,
            height: mainHeight
        }
    }
    const mainchart = chart.createChart('user.minu', 'system.minute', mainLayoutCfg, function (result) {
        // console.log(result)
        if (result.key === 'ArrowUp' || result.key === 'ArrowDown') {
            const curminus = Init.getUserInfo(chartName, 'minuteCharts')
            if (result.key === 'ArrowUp') {
                if (curminus > 1) {
                    Init.setUserInfo(chartName, 'minuteCharts', curminus - 1)
                    Init.changeChart()
                }
            }
            if (result.key === 'ArrowDown') {
                if (curminus < 10) {
                    Init.setUserInfo(chartName, 'minuteCharts', minunums + 1)
                    Init.changeChart()
                }
            }
            // console.log('====', result.key, minunums, Init.getUserInfo(chartName, 'minuteCharts'));
        }
    })
    chart.bindData(mainchart, 'MINUTE')

    const volumeHeight = canvas.height * CLAPI.CH2
    const volumeLoyoutCfg = {
        layout: CLAPI.ChartDef.CHART_LAYOUT,
        config: CLAPI.Utils.mergeJsonOfDeep({ 
                axisX: { lines: getAxisXlines(minunums), display: 'normal', }, 
            }, 
            CLAPI.ChartDef.CHART_MINUVOL),
        rectMain: {
            left: 0,
            top: mainHeight,
            width: mainWidth,
            height: volumeHeight
        }
    }
    const volumechart = chart.createChart('user.minuv', 'system.minute', volumeLoyoutCfg, function (result) {
        //  console.log(result)
    })
    chart.bindData(volumechart, 'MINUTE')
    
    // const boardchart = chart.createChart('user.board', 'system.board', boradLayoutCfg, function (result) {
    //     //  console.log(result)
    // })
    // chart.bindData(boardchart, 'SNAPS')

    Init.systemInfo.curChart.onPaint()
}
function getAxisXlines (minunums) {
    if (minunums < 2) {
        return minunums * 4 - 1
    }
    else if (minunums < 4) {
        return minunums * 2 - 1
    }
    else {
        return minunums - 1
    }
}

export function onLoad (systemInfo) {
    function onLoadData (curKey, inData) {
        
        let cmds = {}
        for (let index = 0; index < inData['info'].datas.length; index++) {
            let curDate = CLAPI.Utils.getMsecDate(inData['info'].datas[index][0]);
            cmds['minute' + index.toString()] = { "cmd" : "get", "subject" : curKey + ".inout_minu1", "service": "iodb/iodate", "info" : {"sub-date" : curDate}}
        }
        Init.sendCommand(
            {   
                cmds : cmds
            },
            function reply(redatas) {
                if (Init.checkDatas(redatas)) {
                    let minunums = 0
                    // minunums = Object.keys(redatas)
                    for (let key in redatas) {
                        minunums++
                        if (inData['minute'] === undefined) {
                            inData['minute'] = redatas[key]
                        } 
                        else {
                            CLAPI.Utils.incrArray(inData['minute'].datas, redatas[key].datas)
                        }
                    }
                    // console.log('====', minunums, inData);
                    drawMinute(inData, minunums)
                }
            });
    }
    let curKey = Init.getcurKey()
    let minuteCharts = Math.min(Init.getUserInfo(chartName, 'minuteCharts', 1), 10)
    
    Init.sendCommand(
        {   
            cmds : {
                "info"   : { "cmd" : "get", "subject" : curKey + ".alpha_info", "service": "merge/ainfo", "info" : {"offset" : -1 * minuteCharts}}
            }
        },
        function reply(datas) {
            if (Init.checkDatas(datas)) {
                Init.setcurKey(curKey);
                onLoadData(curKey, datas)
            }
        });
}

