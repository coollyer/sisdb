import { setBtnEvent } from './load.js';
import { getMockData } from './datas/mockdata.js';
import { CLAPI } from './clchart/cl.api.js'

// 用户信息操作
function saveUserInfo(key, value) {
    if (window.localStorage) {
        window.localStorage.setItem(key, value);
    }
}

function loadUserInfo(key) {
    if (window.localStorage) {
        return window.localStorage.getItem(key);
    }
}

function clearUserInfo() {
    if (window.localStorage) {
        window.localStorage.clear();
    }
}

function removeUserInfo(key) {
    if (window.localStorage) {
        window.localStorage.removeItem(key);
    }
}
// systemInfo.userInfo.newKey 变化
function changeKey() {
    // 检查输入是否合法 合法就传入工作层
    if (systemInfo.userInfo.newKey.length > 0)
    {
        if (systemInfo.userInfo.newKey.length < 6) {
            systemInfo.userInfo.newKey = systemInfo.userInfo.newKey.padStart(6, "0");
        }
        if (systemInfo.userInfo.newKey.length == 8) {
            changeChart()
        }
        else
        {
            // console.log(systemInfo.userInfo.curKey, typeof systemInfo.userInfo.curKey === 'undefined')
            let ch = systemInfo.userInfo.newKey.substring(0, 2)
            if (typeof systemInfo.userInfo.curKey === 'undefined') {
                systemInfo.userInfo.curKey = 'SH600745'
            }
            if (ch === "60" || ch === "68")
            {
                systemInfo.userInfo.newKey = 'SH' + systemInfo.userInfo.newKey
                changeChart()
            }
            if (ch === "00" || ch === "30")
            {
                systemInfo.userInfo.newKey = 'SZ' + systemInfo.userInfo.newKey
                changeChart()
            }
        }   
    }
    else
    {
        systemInfo.userInfo.newKey = undefined
    }
}

function sleep(ms) {
    return new Promise((resolve) => {
      setTimeout(resolve, ms);
    });
}
export function getCanvas() {
    return systemInfo.initInfo.mainCanvas.canvas
}


////////////////////////////
//-------- web服务器 ----- //
////////////////////////////

export function getcurKey() {
    let curKey = systemInfo.userInfo.curKey
    console.log(typeof systemInfo.userInfo.newKey, systemInfo.userInfo.newKey, systemInfo.userInfo.curKey)
    if (typeof systemInfo.userInfo.newKey !== 'undefined') {
        curKey = systemInfo.userInfo.newKey
    }
    else if (typeof systemInfo.userInfo.curKey === 'undefined') {
        systemInfo.userInfo.curKey = 'SH600745'  // 找一个默认值
        curKey = systemInfo.userInfo.curKey
    }
    systemInfo.userInfo.newKey = undefined
    console.log(typeof systemInfo.userInfo.newKey, systemInfo.userInfo.newKey, systemInfo.userInfo.curKey)
    return curKey
}

export function setcurKey(curKey) {
    systemInfo.userInfo.curKey = curKey;
}

export function getUserInfo(kname, key, val) {
    if (systemInfo.userInfo[kname] === undefined) {
        return val
    }
    if (systemInfo.userInfo[kname][key] === undefined) {
        return val
    }
    return systemInfo.userInfo[kname][key]
}
export function setUserInfo(kname, key, val) {
    console.log(systemInfo.userInfo[kname]);
    if (systemInfo.userInfo[kname] === undefined){
        systemInfo.userInfo[kname] = {}
    }
    systemInfo.userInfo[kname][key] = val
    console.log(systemInfo.userInfo[kname]);
}

export function checkDatas(datas) {
    let isok = true;
    for (const name in datas) {
        if (datas[name].tag !== 0) {
            isok = false;
            // console.log(isok, datas[name])
            break;
        }
    }
    return isok;
}

const waitInfo = { cursec : 500, minsec : 500, maxsec : 5000, steps : 1000 }
// 服务器事件操作
export function connectServer() {
    systemInfo.server.ws = new WebSocket(systemInfo.server.address)
    systemInfo.server.ws.onopen = function (e) {
        systemInfo.server.status = 'opened';
        console.log(systemInfo.server.status);
        sendCommand({name : "auth", cmds  : [{ "cmd" : "auth", "info" : {"username" : 'admin', "password" : 'admin5678'}}]}, 
            function reply(datas) {
                systemInfo.server.status = 'authed';
                if (systemInfo.server.style !== 'mock')
                {
                    changeChart()
                }
                console.log(systemInfo.server.status);
        })
        waitInfo.cursec = waitInfo.minsec
    };
    systemInfo.server.ws.onclose = function (e) {
        console.log('ws onclose.')
        let intervalId = setInterval(() => {
            console.log('延迟执行的代码...', waitInfo.cursec);
            // 清除定时器，确保只执行一次
            clearInterval(intervalId);
            connectServer();
            waitInfo.cursec += waitInfo.steps
            if (waitInfo.cursec > waitInfo.maxsec) {
                waitInfo.cursec = waitInfo.maxsec
            }
        }, waitInfo.cursec);
    };
    
    systemInfo.server.ws.onerror = function (evt) {
        console.log('ws onerror.', evt);
    };
    systemInfo.server.ws.onmessage = function (message) {
    
        console.log('===>', typeof message.data, message.data);
    
        if (typeof (message.data) === "string") {
            let start = message.data.indexOf(':');
            let cid = message.data.substr(0, start);
    
            if (systemInfo.server.wait.commands[cid] !== undefined) {
                let data = JSON.parse(message.data.substr(start + 1, message.data.length))
                
                let repv = systemInfo.server.wait.replys[systemInfo.server.wait.commands[cid]]
                if (data.tag !== undefined) {
                    repv['tag'] = data.tag;
                    if (data.tag == 0) {
                        if (isJson(data.info)) {
                            infoData = JSON.parse(data.info)    
                            repv['datas']= infoData.datas
                            repv['fields'] = infoData.fields
                        }
                    }
                    else
                    {
                        repv['info'] = data.info;
                    }     
                }
                else {
                    repv['tag'] = -1
                    repv['info'] = 'ERROR';
                }
            }
            {
                let whole = true;
                for (const cid in systemInfo.server.wait.commands) {
    
                    // console.log('recv : ', cid, systemInfo.server.wait.commands[cid]);
    
                    if (systemInfo.server.wait.replys[systemInfo.server.wait.commands[cid]].tag === undefined) {
                        whole = false;
                        break;
                    }
                }
                if (whole) {
                    // let infoData = {}
                    // if (isJson(datas[0])) {
                    //     infoData = JSON.parse(datas[0])         
                    // }
                    // let dateData = {}
                    // if (isJson(datas[0])) {
                    //     dateData = JSON.parse(datas[1])         
                    // }
                    systemInfo.server.wait.callback(systemInfo.server.wait.replys);
                }
            }
        }
    }
    systemInfo.server.status = 'connect'
    console.log(systemInfo.server.status)
};  
export function checkServer() {
    if (systemInfo.server.status === 'connect') { 
        return true
    }
    return false
};   

let cmdID = 0;
function getCmdID() {
    cmdID++
    return cmdID.toString();
}

// 多个命令组合 当发出的指令全部返回后，就回调函数
export function sendCommand(askcmds, callback) {
    if (systemInfo.server.style === 'mock') {
        callback(getMockData(askcmds.cmds, systemInfo.userInfo.curKey));
        return 
    }
    systemInfo.server.wait = {
        callback: callback,
        commands: {},
        replys: {}
    }
    for (const askname in askcmds.cmds) {
        let cid = getCmdID();
        systemInfo.server.wait.commands[cid] = askname;
        systemInfo.server.wait.replys[askname] = {}

        let sendstr = cid + ':' + JSON.stringify(askcmds.cmds[askname]);

        console.log('<===', sendstr, systemInfo.server.wait.commands);

        systemInfo.server.ws.send(sendstr);
    }
}

export function isJson(str) {
    if (typeof str == 'string') {
        try {
            let obj = JSON.parse(str);
            if (typeof obj == 'object' && obj) {
                return true;
            } else {
                return false;
            }

        } catch (e) {
            console.log('error : ' + str + '!!!' + e);
            return false;
        }
    }
    // console.log('It is not a string!')
}
////////////////////////////
//-------- 初始化事件 ----- //
////////////////////////////
function nextChart() {
    // 这里判断是否切换画面
    const whileCharts = {
       'btKbar' : 'btMarket',
       'btMinute' : 'btKbar',
       'btMarket' : 'btMinute', 
    }
    console.log(systemInfo.userInfo.activeChart, whileCharts[systemInfo.userInfo.activeChart]);
    if (whileCharts[systemInfo.userInfo.activeChart] !== undefined) {
        changeChart(whileCharts[systemInfo.userInfo.activeChart])
    }
}
function setWinEvent() {
    /////////
    // 这个事件在 load 之前发生 此时加载插件数据
    document.addEventListener("DOMContentLoaded", function () {
        // workerJs.forEach(function(jsfile) {
        //     const script = document.createElement('script');
        //     script.src = jsfile;
        //     document.body.appendChild(script);
        //     console.log(jsfile)
        // });
    });
    window.addEventListener('load', function() {
        if (systemInfo.server.style === 'mock')
        {
            changeChart()
        }
    });
    // 以下事件在离开或刷新页面时触发
    window.addEventListener("beforeunload", function (event) {
        // 这里保存参数 
        saveUserInfo('userInfo', JSON.stringify(systemInfo.userInfo));
        // event.preventDefault();
        // event.returnValue = "你有未保存的数据，确定要离开吗？";
    });
    // 处理窗口尺寸变化
    window.onresize = window._debounce(function () {
        {
            let canvas = systemInfo.initInfo.mainCanvas.canvas
            canvas.width = canvas.clientWidth * window.devicePixelRatio
            canvas.height = canvas.clientHeight * window.devicePixelRatio
        }
        {
            let canvas = systemInfo.initInfo.moveCanvas.canvas
            canvas.width = canvas.clientWidth * window.devicePixelRatio
            canvas.height = canvas.clientHeight * window.devicePixelRatio
        }
        if (typeof systemInfo.curChart !== 'undefined') {
            // systemInfo.curChart.onPaint()
            changeChart()
        }
    }, 100)
    // 接受外部输入
    var inputBox = document.getElementById('inputBox');
    document.addEventListener('keydown', function (event) {
        // 激活输入框
        console.log(event, event.key, event.keyCode)
        if (event.key >= '0' && event.key <= '9') {
            inputBox.focus();
        }  
        if (document.activeElement === inputBox) {
            if (event.key === 'Enter') {
                console.log('Enter 键被按下了');
                if (inputBox.value.length > 0) {
                    inputBox.focus();
                    systemInfo.userInfo.newKey = inputBox.value; // 只有当数据成功才会设置到 curKey 否则设置为 ''
                    changeKey()
                    inputBox.value = '';
                    inputBox.blur(); // 取消焦点
                }
            }
        }
        else {
            if (event.key === 'Enter') {
                nextChart()
            }
            else 
            {
                // 这里强制传递一下按键事件 鼠标事件下层可以自动获取
                const keydownEvent = new KeyboardEvent('keydown', {
                    key: event.key,
                    code: event.code,
                    keyCode: event.keyCode
                });
                systemInfo.initInfo.eventCanvas.dispatchEvent(keydownEvent);
            }
        }
    });
}
export function sendKeydownEvent(event) {
    const keydownEvent = new KeyboardEvent('keydown', {
        key: event.key,
        code: event.code,
        keyCode: event.keyCode
    });
    document.dispatchEvent(keydownEvent);
}


export async function changeChart(mName) {
    console.log('===+++===', mName, systemInfo.userInfo.activeChart)
    if (mName === undefined) {
        if (systemInfo.userInfo['activeChart'] !== undefined) {
            mName = systemInfo.userInfo.activeChart
        }
        else
        {
            mName = 'btKbar'
        }
    }
    console.log(mName, typeof mName, systemInfo.methods, Object.keys(systemInfo.methods), Object.getOwnPropertyNames(systemInfo.methods), 'kbar' in systemInfo.methods)
    if (mName in systemInfo.methods) {
        systemInfo.userInfo.activeChart = mName
        systemInfo.methods[mName]()
        
    }
    // try {
    //     systemInfo.methods[mName]()
    // } catch (e) {
    //     console.log('no method ' + mName + '   :' + e);
    // }
}

// ////////////////////////////
// 所有初始化变量都在这里
// systemInfo 为唯一全局变量
// systemInfo.userInfo 为需要保存的数据 kv 结构
// systemInfo.server 为连接服务器的句柄
// ////////////////////////////

//-------- 初始化开始 ----- //
export const systemInfo = {
    methods : {},
    server : {},
    userInfo : {},
    initInfo : {
        mainCanvas: {
            canvas  : document.getElementById('mainChart')
        },
        moveCanvas: {
            canvas  : document.getElementById('moveChart')
        },
        eventCanvas : document.getElementById('moveChart'),
        scale: window.devicePixelRatio,
        axisPlatform: 'web'
    }
}

var serverAddress = 'ws://127.0.0.1:7329'
// var serverAddress = 'ws://127.0.0.1:7331'

systemInfo.server.style = 'mock' 
// systemInfo.server.style = 'ws' 

async function startSystem() {
    // 设置画布参数
    // systemInfo.initInfo.mainCanvas.canvas = document.getElementById('mainChart')
    // systemInfo.initInfo.mainCanvas.context = systemInfo.initInfo.mainCanvas.canvas.getContext('2d')
    // systemInfo.initInfo.moveCanvas.canvas = document.getElementById('moveChart')
    // systemInfo.initInfo.moveCanvas.context = systemInfo.initInfo.moveCanvas.canvas.getContext('2d')
    // 这里创建画图类
    systemInfo.curChart = new CLAPI.manageChart(systemInfo.initInfo)
    // 设置接受事件的cancas
    // systemInfo.initInfo.eventCanvas = systemInfo.initInfo.moveCanvas.canvas
    // 设置事件
    setWinEvent()
    setBtnEvent(systemInfo)
    // 启动服务器
    systemInfo.server.address = serverAddress;
    if (systemInfo.server.style !== 'mock')
    {
        connectServer();
    }
    // 加载用户配置
    let cfg = loadUserInfo('userInfo')
    // console.log('userInfo', cfg)
    if (isJson(cfg))
    {
        systemInfo.userInfo = JSON.parse(cfg)
    }
    else
    {
        systemInfo.userInfo = {}
    }
    
    console.log('userInfo', systemInfo.userInfo)
}

// 这里直接执行
startSystem()


