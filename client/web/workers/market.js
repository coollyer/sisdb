import * as Init from '../init.js';
import {
    CLAPI
} from '../clchart/cl.api.js'
import * as DATAS from '../datas/transdata.js'

// 画日线
function drawKbar(inData) {

    console.log('Draw ====> Market Table', inData)

    const canvas = Init.getCanvas()

    // 设置为单一窗口 并返回第一个窗口
    const chart = Init.systemInfo.curChart.initChart(['Market'])
    // 得到第一个窗口

    // 需要把所有的股票代码 按 stkInfo['SH600600'] = {} 这种方式生成一个 stkInfo 字典传入 chart 中
    // 例子 SH600600 = { static : [[]], frights : [[]] }
    // 按股票名称写入 这个需要统一起来 不再支持 setBaseInfo setFrightInfo 
    const keyInfo = DATAS.mAinfoToKeyInfo(inData['infos'])
    chart.setKeyInfo(keyInfo)
    console.log(keyInfo);

    const FIELD_FTREND = {
        index: 0,
        code: 1,
        name: 2,
        newp: 3,
    }
    // 增加其他字段的信息
    for (let index = 1; index < 20; index++) {
        const name = 'factor' + index.toString()
        FIELD_FTREND[name] = Object.keys(FIELD_FTREND).length
    }
    // console.log(FIELD_FTREND)

    let makets = []
    for (let index = 0; index < inData['ftrend'].datas.length; index++) {
        if (inData['ftrend'].datas[index][4] == 1) {
            const code = inData['ftrend'].datas[index][0]
            const name = keyInfo[code].static.name
            makets.push(
                [makets.length + 1,
                    code,
                    name,
                    inData['ftrend'].datas[index][3],
                    inData['ftrend'].datas[index][12],
                    inData['ftrend'].datas[index][14],
                    inData['ftrend'].datas[index][16],
                    inData['ftrend'].datas[index][19],
                    inData['ftrend'].datas[index][24],
                    inData['ftrend'].datas[index][26],
                    inData['ftrend'].datas[index][34],
                    inData['ftrend'].datas[index][36],
                    inData['ftrend'].datas[index][40],
                    inData['ftrend'].datas[index][41],
                    inData['ftrend'].datas[index][42],
                    inData['ftrend'].datas[index][43],
                    inData['ftrend'].datas[index][72],
                    inData['ftrend'].datas[index][76]
                ])
        }
    }
    chart.setData('MARKET', FIELD_FTREND, makets)

    let curDate = CLAPI.Utils.getMsecDate(inData['info'].datas[0][0]);
    const tableInfo = [{
            title: '筛选',
            label: 'index',
            len: 4,
            align: 'center', // align 默认为'right'
            sort: 'resume', // sort默认为 number  resume表示原来的序列
            getclr: 'getIntClr',
            getvar: 'getIntVar',
        },
        {
            title: curDate.toString(),
            label: 'code',
            len: 8,
            align: 'left', // alignTxt 默认为'right'
            sort: 'string',
            getclr: 'getCodeClr',
            getvar: 'getStrVar',
        },
        {
            title: "名称",
            label: 'name',
            len: 8,
            align: 'left', // alignTxt 默认为'right'
            sort: 'resume',
            getclr: 'getNameClr',
            getvar: 'getStrVar',
        },
        {
            title: '最新价',
            label: 'newp',
            change: true, // change:true 表示如果数值有变化，显示动画效果
            getclr: 'getPrcClr',
            getvar: 'getPrcVar',
        },
        {
            title: 'factor1',
            label: 'factor1',
            getclr: 'getZeroClr',
        },
        {
            title: 'factor2',
            label: 'factor2',
            getclr: 'getFltClr', // 默认调用 getFltClr 表示浮点数
            getvar: 'getPctVar', // 按百分比获取数值
        },
        {
            title: 'factor3',
            label: 'factor3',
            getclr: 'getFltClr', // 默认调用 getFltClr 表示浮点数
            getvar: 'getPctVar', // 按百分比获取数值
        },
        {
            title: 'factor4',
            label: 'factor4',
            getclr: 'getVolClr',
            getvar: 'getVolVar',
        },
        {
            title: 'factor5',
            label: 'factor5',
            getclr: 'getIntClr',
            getvar: 'getIntVar',
        },
        {
            title: 'factor6',
            label: 'factor6',
            getclr: 'getIntClr',
            getvar: 'getIntVar',
        }

    ];
    // 增加其他字段的信息
    for (let index = 7; index < 20; index++) {
        const name = 'factor' + index.toString()
        tableInfo.push({
            title: name,
            label: name
        })
    }
    console.log(tableInfo)

    // 设置界面信息和绑定数据
    const mainLayoutCfg = {
        layout: {
            offset: {
                top: 5,
                bottom: 5,
                left: 5,
                right: 5
            }
        },
        config: {
            fieldlock: 4, // 按左键时，前面3个字段不动        
            curKeyIdx: 1, // 用于检索curKey的字段索引
        },
        tableInfo: tableInfo,
        rectMain: {
            left: 0,
            top: 0,
            width: canvas.width,
            height: canvas.height
        }
    }
    const mainchart = chart.createChart('user.market', 'system.table', mainLayoutCfg, function (result) {
        // 这里应该返回交互信息 比如按钮按下 光标位置等
        if (result.event === 'keydown') {
            // console.log(result, makets);
            if (result.keyCode === 13) {
                Init.setcurKey(makets[result.curIndex][1])
                chart.setlinkInfo('curKey', makets[result.curIndex][1])
                Init.sendKeydownEvent(result)
            }
        }
        console.log('MARKET', result, makets[result.curIndex][1], Init.getcurKey())
    })
    chart.setlinkInfo('curKey', Init.getcurKey())
    chart.bindData(mainchart, 'MARKET')

    Init.systemInfo.curChart.onPaint()
}

export function onLoad() {
    function onLoadData(inData) {
        // const curDate = CLAPI.Utils.getMsecDate(inData['info'].datas[0][0]);
        const curKey = Init.getcurKey();
        const curDate = 20250303
        Init.sendCommand({
                cmds: {
                    // 同时获取数据会服务器崩溃 后面查查
                    // 'snapshot' : { "cmd" : "get", "subject" : curKey + ".stk_snapshot", "service": "snodb", "info" : {"sub-date" : curDate}},
                    // 'alpha_info' : { "cmd" : "get", "subject" : curKey + ".alpha_info", "service": "atdb/minu1", "info" : {"sub-date" : curDate}},
                    // "info" : { "cmd" : "get", "subject" : curKey + ".alpha_info", "service": "merge/ainfo", "info" : {"offset" : -1}},
                    "infos": {
                        "cmd": "get",
                        "subject": "CN000000.alpha_info",
                        "service": "atdb/minu1",
                        "info": {
                            "sub-date": curDate
                        }
                    },
                    "ftrend": {
                        "cmd": "get",
                        "subject": "db_params",
                        "service": "../cailu/bin/ftrend/worker",
                        "info": {
                            "sub-date": curDate
                        }
                    }
                }
            },
            function reply(datas) {
                if (Init.checkDatas(datas)) {
                    datas['info'] = inData['info']
                    drawKbar(datas)
                }
            });
    }
    Init.sendCommand({
            cmds: {
                "info": {
                    "cmd": "get",
                    "subject": "SH000001.alpha_info",
                    "service": "merge/ainfo",
                    "info": {
                        "offset": -1
                    }
                }
            }
        },
        function reply(datas) {
            console.log(datas)
            if (Init.checkDatas(datas)) {
                // Init.setcurKey(curKey);
                onLoadData(datas)
            }
        });
}


// 注册方法到参数表