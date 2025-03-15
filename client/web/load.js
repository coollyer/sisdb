
import * as Init from './init.js';

import * as minute from './workers/minute.js';
import * as market from './workers/market.js';
import * as kbar from './workers/kbar.js';

function fromMenuToChart(mname) {
    document.getElementById(mname).addEventListener('click', function () {
        Init.changeChart(mname);
        document.activeElement.blur()
    });
}
// 这里做按钮和图形的对应关系
export function setBtnEvent(systemInfo) {
    fromMenuToChart('btMinute')
    fromMenuToChart('btMarket')
    fromMenuToChart('btKbar')
    // fromMenuToChart('btFbar')
    // fromMenuToChart('btEmv')
    
    // 这里做函数映射
    registerMenu('btMinute', minute.onLoad)
    registerMenu('btMarket', market.onLoad)
    registerMenu('btKbar'  , kbar.onLoad)
}
function registerMenu(mname, method) {
    Init.systemInfo.methods[mname] = method
}
