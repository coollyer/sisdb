
import { CLAPI } from '../clchart/cl.api.js'

// 画日线
export function mAinfoToKeyInfo(indata) {
    // keyInfo['SH600600'] = {} 这种方式生成一个 keyInfo 字典传入 chart 中
    // 例子 SH600600 = { static : {}, frights : [[]] }

    let keyInfo = {}
    for (let index = 0; index < indata.datas.length; index++) {
        const key = CLAPI.getData(indata, 'code', index)
        keyInfo[key] = {
            static : {
                curdate: CLAPI.Utils.getMsecDate(CLAPI.getData(indata, 'msec', index)),
                name   : CLAPI.getData(indata, 'name', index),
                coindot: 2,
                prcunit: 1,
                volunit: 1,
                mnyunit: 1,
                style  : CLAPI.getData(indata, 'style', index) ? CLAPI.DatasDef.STOCK_TYPE_STK : CLAPI.DatasDef.STOCK_TYPE_IDX,
                agoprc : CLAPI.getData(indata, 'agoprc', index),
                stopmax: CLAPI.getData(indata, 'stopup', index),
                stopmin: CLAPI.getData(indata, 'stopdn', index),
                wstake : CLAPI.getData(indata, 'wstake', index),
                fstake : CLAPI.getData(indata, 'fstake', index),
                income : CLAPI.getData(indata, 'income', index) / CLAPI.getData(indata, 'wstake', index),
            },
        }
    }
    return keyInfo
}

// 只有一个股票
export function AinfoToKeyInfo(indata) {
    // keyInfo['SH600600'] = {} 这种方式生成一个 keyInfo 字典传入 chart 中
    // 例子 SH600600 = { static : {}, frights : [[]] }

    let keyInfo = {}
    const index = -1
    const key = CLAPI.getData(indata, 'code', index)
    keyInfo[key] = {
        static : {
            curdate: CLAPI.Utils.getMsecDate(CLAPI.getData(indata, 'msec', index)),
            name   : CLAPI.getData(indata, 'name', index),
            coindot: 2,
            prcunit: 1,
            volunit: 100,
            mnyunit: 100,
            style  : CLAPI.getData(indata, 'style', index) ? CLAPI.DatasDef.STOCK_TYPE_STK : CLAPI.DatasDef.STOCK_TYPE_IDX,
            agoprc : CLAPI.getData(indata, 'agoprc', index),
            stopmax: CLAPI.getData(indata, 'stopup', index),
            stopmin: CLAPI.getData(indata, 'stopdn', index),
            wstake : CLAPI.getData(indata, 'wstake', index),
            fstake : CLAPI.getData(indata, 'fstake', index),
            income : CLAPI.getData(indata, 'income', index) / CLAPI.getData(indata, 'wstake', index),
        },
    }
    if (indata.datas.length > 1) {
        keyInfo[key].frights = []
        let fright = 1.0
        for (let index = indata.datas.length - 1; index >= 0; index--) {
            fright *= CLAPI.getData(indata, 'fright', index) 
            keyInfo[key].frights.unshift([CLAPI.Utils.getMsecDate(CLAPI.getData(indata, 'msec', index)), fright])
        }         
    }
    return keyInfo
}

export function AdateToAxisXinfo(indata, nfield) {
    let oInfos = []
    // 这里要生成X轴的数据 
    for (let index = 0; index < indata.datas.length; index++) {
        oInfos.push(CLAPI.Utils.getMsecDate(CLAPI.getData(indata, nfield, index))) 
    }
    return oInfos
}

// 格式化 msec_t 为指定字符串
export function fromMsecToMinu(msec, tradetime) {
    const mtime = new Date(msec)
    let curminu = mtime.getHours() * 100 + mtime.getMinutes()
    const start = tradetime[0][0]
    const stop  = tradetime[tradetime.length - 1][1]
    if (curminu <= start) {
        return CLAPI.Utils.fromMinuteToStr(start)
    } 
    else if (curminu >= stop) {
        return CLAPI.Utils.fromMinuteToStr(stop)
    }
    return CLAPI.Utils.fromMinuteToStr(curminu)
}

export function IominuToAxisXinfo(indata, nfield) {
    let timeInfo = []
    let dateInfo = []
    let curdate = 0
    let agodate = 0
    let curminu = 0
    let agominu = 0
    // 这里要生成X轴的数据 
    for (let index = 0; index < indata.datas.length; index++) {
        const vmsec = CLAPI.getData(indata, nfield, index)
        curminu = CLAPI.DataUtils.getTradeMsecToIndex(vmsec, CLAPI.DatasDef.STOCK_TRADETIME);
        curdate = CLAPI.Utils.getMsecDate(vmsec)
        if (curdate !== agodate) {
            dateInfo.push(curdate)
            agodate = curdate
        }
        if (curminu === agominu) {
            timeInfo.pop()
        }
        timeInfo.push(fromMsecToMinu(vmsec, CLAPI.DatasDef.STOCK_TRADETIME)) 
        agominu = curminu
    }
    return {timeInfo, dateInfo}
}

export function IominuToMinute(indata) {
    let minus = []
    let curdate = 0
    let agodate = 0
    let curminu = 0
    let agominu = -1
    let incminu = 0
    let curv = 0
    let curm = 0
    let sumbigm = 0
    let incbigm = 0
    let curbigm = 0
    let agoaskm = 0
    let agobidm = 0

    for (let index = 0; index < indata.datas.length; index++) {
        
        curminu = CLAPI.DataUtils.getTradeMsecToIndex(indata.datas[index][0], CLAPI.DatasDef.STOCK_TRADETIME);
        curdate = CLAPI.Utils.getMsecDate(indata.datas[index][0])
        if (curdate !== agodate) {
            incminu += agominu
            incbigm += sumbigm
            agominu = -1
            sumbigm = 0
            curbigm = 0
            agoaskm = 0
            agobidm = 0
            agodate = curdate
        }
        if (curminu === agominu) {
            minus.pop()
        }
        if (minus.length > 0) {
            let agov = minus.at(-1)
            curv = indata.datas[index][10] - agov[5]
            curm = indata.datas[index][9] - agov[6]
        }
        else
        {
            curv = indata.datas[index][10]
            curm = indata.datas[index][9]
        }
        let askm = 0
        let bidm = 0
        for (let fi = 3; fi < 8; fi++) {
            askm += CLAPI.getData(indata, 'oaskm' + fi, index) + CLAPI.getData(indata, 'iaskm' + fi, index)
            bidm += CLAPI.getData(indata, 'obidm' + fi, index) + CLAPI.getData(indata, 'ibidm' + fi, index)
        }
        curbigm = (bidm - askm) - (agobidm - agoaskm)
        sumbigm = (bidm - askm)
        agoaskm = askm
        agobidm = bidm
        minus.push(
            [indata.datas[index][0],
            indata.datas[index][3],
            indata.datas[index][4],
            indata.datas[index][5],
            indata.datas[index][6],
            indata.datas[index][10],
            indata.datas[index][9],
            curv,
            curm,
            curbigm / 100,
            (sumbigm + incbigm) / 100,
            curminu])
        agominu = curminu
    }
    return minus
}

export function TrendToBIGM(indata, maindata) {
    let bigms = []
    for (let index = 0; index < indata.datas.length; index++) {
        const stoped = CLAPI.getData(indata, 'stoped', index)
        if (stoped !== 1) {
            continue
        }
        bigms.push(
            [CLAPI.Utils.getMsecDate(CLAPI.getData(indata, 'msecstamp', index)),
            CLAPI.getData(indata, 'curr_askm', index),
            CLAPI.getData(indata, 'curr_bidm', index),
            CLAPI.getData(indata, 'curr_bigm', index),
            CLAPI.getData(indata, 'curr_askr', index),
            CLAPI.getData(indata, 'curr_bidr', index),
            ])
    }
    const subs = maindata.length - bigms.length
    for (let index = subs; index >=0 ; index--) {
        bigms.unshift([maindata[index][0], 0,0,0,0,0])
    }
    return bigms
}

export function IodateToBIGM(indata, maindata) {
    let bigms = []
    // let mid_askm = 0
    // let mid_bidm = 0
    // let min_askm = 0
    // let min_bidm = 0

    let mainindex = 0
    for (let index = 0; index < indata.datas.length; index++) {
        const curdate = CLAPI.Utils.getMsecDate(CLAPI.getData(indata, 'msec', index))
        while (curdate > maindata[mainindex][0]) {
            bigms.unshift([maindata[mainindex][0], 0,0,0,0,0])
            console.log(curdate, maindata[mainindex][0]);
            mainindex++
        }
        if (curdate < maindata[mainindex][0]) {
            console.log(curdate, maindata[mainindex][0]);
            continue
        }
        let max_askm = 0
        let max_bidm = 0
        for (let mi = 3; mi < 8; mi++) {
            max_askm += CLAPI.getData(indata, 'oaskm' + mi.toString(), index)
            max_askm += CLAPI.getData(indata, 'iaskm' + mi.toString(), index)
            max_bidm += CLAPI.getData(indata, 'obidm' + mi.toString(), index)
            max_bidm += CLAPI.getData(indata, 'ibidm' + mi.toString(), index)
        }
        bigms.push(
            [curdate,
                max_askm,
                max_bidm,
                max_bidm - max_askm,
                max_askm / CLAPI.getData(indata, 'summ', index),
                max_bidm / CLAPI.getData(indata, 'summ', index),
            ])
        mainindex++
    }
    return bigms
}