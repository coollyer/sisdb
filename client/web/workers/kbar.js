import * as Init from '../init.js';
import { CLAPI } from '../clchart/cl.api.js'
import * as DATAS from '../datas/transdata.js'

// 画日线
function drawKbar (inData) {

    console.log('Draw ====> Kbar Line', inData)

    const canvas = Init.getCanvas()
    // 设置为单一窗口 并返回第一个窗口
    const chart = Init.systemInfo.curChart.initChart(['Kbar'])
    // 得到第一个窗口

    // console.log(JSON.stringify(inData['info'].datas));
    // console.log(JSON.stringify(inData['kbar'].datas));
    // 初始化数据
    // 外部要完全可控才行 数据和画图完全隔离
    // 1. 设置树结构 
    // 2. 设置每个树的坐标系
    // 3. 设置每个分支的数据 和 画图类型
    // 4. 设置交互回调的信息

    // console.log(charts)
    chart.setlinkInfo('curKey', Init.getcurKey())
    
    const keyInfo = DATAS.AinfoToKeyInfo(inData['minfo'])
    chart.setKeyInfo(keyInfo)
    console.log(keyInfo);

    const axisXinfo = DATAS.AdateToAxisXinfo(inData['kbar'], 'msec')
    // 设置坐标显示信息
    chart.setAxisXinfo(axisXinfo)

    const FIELD_KBAR = {
        date: 0,
        open: 1,
        maxp: 2,
        minp: 3,
        newp: 4,
        sumv: 5,
        summ: 6,
        clr : 7,  
    }
    let kbars = []
    for (let index = 0; index < inData['kbar'].datas.length; index++) {
        clr = 'red'
        if (parseFloat(inData['kbar'].datas[index][3]) > parseFloat(inData['kbar'].datas[index][6])) {
            clr = 'green'
        }
        kbars.push(
            [CLAPI.Utils.getMsecDate(inData['kbar'].datas[index][0]),
            inData['kbar'].datas[index][3],
            inData['kbar'].datas[index][4],
            inData['kbar'].datas[index][5],
            inData['kbar'].datas[index][6],
            inData['kbar'].datas[index][10],
            inData['kbar'].datas[index][9],
            clr])
    }
    chart.setData('KBAR', FIELD_KBAR, kbars)
    

    // 设置界面信息和绑定数据
    const mainHeight = canvas.height * CLAPI.CH1
    // let mainWidth = Math.max(canvas.width * 0.65, canvas.width - 400)
    const mainWidth = canvas.width

    const mainLayoutCfg = {
        layout: {
            offset: {
                top: 5,
                bottom: 5,
                left: 5,
                right: 10
            }
        },
        config: CLAPI.Utils.mergeJsonOfDeep({
                title: {
                    label: keyInfo[Init.getcurKey()].static.name,
                },
            },
            CLAPI.ChartDef.CHART_KBAR),
        rectMain: {
            left: 0,
            top: 0,
            width: mainWidth,
            height: mainHeight
        }
    }
    // 设置 DAY 为该表的数据源
    // 其他为辅助数据 实际运算时
    const mainchart = chart.createChart('user.kbar', 'system.chart', mainLayoutCfg, function (result) {
        // 这里应该返回交互信息 比如按钮按下 光标位置等
        // console.log('KBAR', result)
    })
    chart.bindData(mainchart, 'KBAR')

    const volumeHeight = canvas.height * CLAPI.CH2
    const volumeLoyoutCfg = {
        layout: {
            offset: {
                top: 5,
                bottom: 5,
                left: 5,
                right: 10
            }
        },
        config: CLAPI.Utils.mergeJsonOfDeep({ 
            axisX: { display: 'normal', }, 
        }, 
        CLAPI.ChartDef.CHART_VBAR),
        rectMain: {
            left: 0,
            top: mainHeight,
            width: mainWidth,
            height: volumeHeight
        }
    }
    const volumechart = chart.createChart('user.vbar', 'system.chart', volumeLoyoutCfg, function (result) {
        // console.log('VBAR', result)
    })
    chart.bindData(volumechart, 'KBAR')

    Init.systemInfo.curChart.onPaint()
}

export function onLoad() {
    function onLoadData (curKey, inData) {
        const curDate = CLAPI.Utils.getMsecDate(inData['info'].datas[0][0]);
        Init.sendCommand(
            {   
                cmds : {
                    // "ftrend" : { "cmd" : "get", "subject" : "db_params", "service": "../cailu/bin/ftrend/worker", "info" : {"sub-date" : curDate}}
                    // 'bigm' : { "cmd" : "get", "subject" : curKey + ".inout_date", "service": "iodb/iodate", "info" : {"sub-date" : curDate}},
                    // 'bigm' : { "cmd" : "get", "subject" : curKey + ".db_params", "service": "../cailu/bin/ftrend/worker/trend", "info" : {"sub-date" : curDate}},
                    "minfo" : { "cmd" : "get", "subject" : curKey + ".alpha_info", "service": "merge/ainfo", "info" : {"offset" : 0}},
                    "kbar" : { "cmd" : "get", "subject" : curKey + ".alpha_date", "service": "merge/ainfo", "info" : {"offset" : 0}}
                }
            },
            function reply(datas) {
                if (Init.checkDatas(datas)) {
                    drawKbar(datas)
                }
            });
    }
    let curKey = Init.getcurKey()
    Init.sendCommand(
        {   
            cmds : {
                "info"   : { "cmd" : "get", "subject" : curKey + ".alpha_info", "service": "merge/ainfo", "info" : {"offset" : -1}}
            }
        },
        function reply(datas) {
            console.log(datas)
            if (Init.checkDatas(datas)) {
                Init.setcurKey(curKey);
                onLoadData(curKey, datas)
            }
        });
}

// 注册方法到参数表
