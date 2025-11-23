let socket = new WebSocket("ws://192.168.0.7/ws");
const ip = "http://192.168.0.7";

const sections = document.querySelectorAll('.sections');
const getActive = document.getElementById('active');
const display = document.getElementById('display-container');
const homeButtons = document.querySelectorAll('.homeButton');
const initalContent = document.getElementById('startContent');
const template = document.getElementById('alarmObjectTemp');

let alarms = [];
let grindData = [];
let grindState = false;
let timer = false;
let min = 0;
let secs = 0;
let timeString;
let timerID = null;
let currentScreen = 'home';
let nextAlarmWatcherStarted = false;
let lastAlarmCheckMinute = null;
let infoIntervalId = null;

const sectionContent = [];

const blankAlarm = `
<div class="alarmObjectContainer">
    <div class="alarmObject">
        <p class="alarmName"></p>
        <div class="alarmData">
            <h2 class="alarmHour"></h2>
            <div class="alarmToggle">
                <label class="switch">
                    <input class="alarmCheckbox" type="checkbox" onchange="toggleAlarm(this)" hidden>
                    <span class="slider"></span>
                </label>
            </div>
        </div>
        <div class="alarmData">
            <p class="alarmRepeat"></p>
            <p class="alarmThickness"></p>
        </div>
    </div>
    <div class="editMode">
        <img src="../assets/trash.png">
    </div>
</div>

`;

const method = 'moka';

async function loadSectionPartials() {
    const files =
    {
        home: '/html/home.html',
        schedule: '/html/schedule.html',
        info: '/html/info.html'
    };

    const entries = await Promise.all(
        Object.entries(files).map(async ([key, url]) => {
            const res = await fetch(url);
            if (!res.ok) throw new Error(`Failed to load ${url}: ${res.status}`);
            const html = await res.text();
            return [key, html];
        })
    );

    entries.forEach(([k, html]) => sectionContent[k] = html);
}

function adjustTime(arrow, timeReset, order) {
    let timeElement = arrow.closest(".getTimeContainer").children[1];
    let time = Number(timeElement.innerText);
    let reset;

    if (timeReset == 'h') reset = 23;
    else if (timeReset == 'm') reset = 59;

    if (order == 's') time = time + 1;
    else if (order == 'm') time = time - 1;
    if (time > reset) time = 0;
    if (time < 0) time = reset;

    timeElement.innerText = String(time).padStart(2, '0');
}

async function talk2ESP32(method, route, jsonData) {
    const address = ip + route;

    const labels =
    {
        method: method.toUpperCase(),
        headers: { "Content-Type": "application/json" },
    }

    if (labels.method !== "GET" && labels.method !== "HEAD" && jsonData !== undefined) {
        labels.body = JSON.stringify(jsonData);
    }

    try {
        const res = await fetch(address, labels);
        if (!res.ok) {
            throw new Error(`HTTP ${res.status} ${res.statusText}`);
        }

        const ct = res.headers.get('content-type') || '';
        if (ct.includes('application/json')) {
            const data = await res.json();
            console.log("ESP32 responded with:", data);
            return data;
        } else {
            const text = await res.text();
            console.log("ESP32 responded with:", text);
            return text;
        }
    } catch (err) {
        console.error("sendToESP32 error:", err);
        throw err;
    }
}

function loadContent(content) {
    switch (content) {
        case 'home':
            currentScreen = 'home';
            display.innerHTML = sectionContent['home'];
            if (grindState) {
                document.getElementById('startContent').classList.remove('show');
                document.getElementById('grindMeasurements').classList.add('show');
                document.getElementById('measurementLabel').classList.add('show');
                document.getElementById('pwrButton').innerText = "OFF";
            }
            else {
                document.getElementById('startContent').classList.add('show');
                document.getElementById('grindMeasurements').classList.remove('show');
                document.getElementById('measurementLabel').classList.remove('show');
                document.getElementById('pwrButton').innerText = "ON";
            }

            for (let i = 0; i < grindData.length; i++) {
                insertRecent(grindData[i].duration, grindData[i].date, grindData[i].power);
            }
            function computeAndShowNextAlarm() {
                if (!Array.isArray(alarms) || alarms.length === 0) {
                    const nName = document.getElementById('nextAlarm');
                    const nRem = document.getElementById('nextAlarmRemains');
                    if (nName) nName.innerText = "———————";
                    if (nRem) nRem.innerText = "";
                    return;
                }

                const now = new Date();
                const reference = now.getHours() * 60 + now.getMinutes();
                const candidates = [];
                for (let i = 0; i < alarms.length; i++) {
                    const a = alarms[i];
                    if (!a || !a.status) {
                        candidates.push({ index: i, diff: Infinity });
                        continue;
                    }
                    const alarmMinutes = Number(a.timeH) * 60 + Number(a.timeM);
                    let diff = alarmMinutes - reference;
                    if (diff < 0) diff += 1440;
                    candidates.push({ index: i, diff });
                }

                candidates.sort((a, b) => a.diff - b.diff);
                const next = candidates[0];
                if (!next || !isFinite(next.diff)) return;

                const a = alarms[next.index];
                const El = document.getElementById('nextAlarm');
                const remEl = document.getElementById('nextAlarmRemains');
                if (El) El.innerText = a.name + " at " + String(Number(a.timeH)).padStart(2, '0') + ":" + String(Number(a.timeM)).padStart(2, '0');

                let remHours = Math.floor(next.diff / 60);
                let remMinutes = next.diff % 60;
                let remainsString = "";
                if (remHours > 0) remainsString = String(remHours) + "h ";
                remainsString += String(remMinutes).padStart(2, '0') + "m remaining";
                if (remEl) remEl.innerText = remainsString;
            }

            if (!nextAlarmWatcherStarted) {
                nextAlarmWatcherStarted = true;
                lastAlarmCheckMinute = new Date().getMinutes();
                computeAndShowNextAlarm();
                setInterval(() => {
                    const now = new Date();
                    const m = now.getMinutes();
                    if (m !== lastAlarmCheckMinute) {
                        lastAlarmCheckMinute = m;
                        computeAndShowNextAlarm();
                    }
                }, 1000);
            } else {
                computeAndShowNextAlarm();
            }
            console.log(grindData);
            break;
        case 'schedule':
            currentScreen = 'schedule';
            const checkbox = document.querySelectorAll('.alarmObject .alarmCheckbox');

            document.getElementById('getTime').querySelectorAll('.getTimeButton').forEach((element) => {
                element.innerText = "00";
            });

            for (let i = 0; i <= alarms.length; i++) {
                if (alarms[i] != undefined) {
                    if (alarms[i].status) createAlarmObject(i, alarms[i].name, String(alarms[i].timeH).padStart('2', 0), String(alarms[i].timeM).padStart('2', 0), alarms[i].repeatS);
                    else createAlarmObject(i, alarms[i].name, String(alarms[i].timeH).padStart('2', 0), String(alarms[i].timeM).padStart('2', 0), alarms[i].repeatS, false);
                }
            }
            console.log(alarms);
            break;
        case 'info':
            currentScreen = 'info';
            talk2ESP32("GET", "/info").then((data) => {
                let timeSum = 0;
                let powerSum = 0;

                for(let i=0 ; i<grindData.length ; i++)
                {
                    let hour = Number(String(grindData[i].duration).substring(0, String(grindData[i].duration).indexOf(":")));
                    let minute = Number(String(grindData[i].duration).substring(String(grindData[i].duration).indexOf(":")+1, String(grindData[i].duration).length));

                    powerSum += Number(grindData[i].power);
                    timeSum += (hour*60) + minute;
                }

                document.getElementById('totalPower').innerText = Math.round(powerSum) + " W";
                document.getElementById('totalGrindTime').innerText = seconds2minutes(timeSum);
                document.getElementById('averageGrindTime').innerText = seconds2minutes(timeSum/grindData.length);

                document.getElementById('ssid').innerText = data.ssid;
                document.getElementById('ip').innerText = data.ip;
                document.getElementById('mac').innerText = data.mac;
                document.getElementById('uptime').innerText = seconds2minutes(data.uptime / 1000, true);

                let stringth;
                if (Number(data.signalStrength) >= -30) stringth = "Excellent" + " (" + data.signalStrength + " dBm)";
                else if (Number(data.signalStrength) <= -67 && Number(data.signalStrength) >= -80) stringth = "Moderate" + " (" + data.signalStrength + " dBm)";
                else if (Number(data.signalStrength) <= -80 && Number(data.signalStrength) >= -90) stringth = "Weak" + " (" + data.signalStrength + " dBm)";
                else if (Number(data.signalStrength) <= -90) stringth = "Very weak" + " (" + data.signalStrength + " dBm)";
                document.getElementById('signalStrength').innerText = stringth;
            });
            break;
        default:
            break;
    }
}

function toggleStart() {
    let startData =
    {
        "status": null,
    };

    grindState = !grindState;
    timer = grindState;

    if (grindState) {
        startData.status = "on";
        secs = -1;
        min = 0;
        grindTimer();
        document.getElementById('startContent').classList.remove('show');
        document.getElementById('grindMeasurements').classList.add('show');
        document.getElementById('measurementLabel').classList.add('show');
        document.getElementById('pwrButton').innerText = "OFF";
    }
    else {
        startData.status = "off";
        document.getElementById('startContent').classList.add('show');
        document.getElementById('grindMeasurements').classList.remove('show');
        document.getElementById('measurementLabel').classList.remove('show');
        document.getElementById('pwrButton').innerText = "ON";
        clearTimeout(timerID);
        timerID = null;
    }

    talk2ESP32("POST", "/on", startData);
}

function grindTimer() {
    timerID = setTimeout(grindTimer, 1000);
    if (timer) {
        secs++;
        if (secs >= 60) {
            secs = 0;
            min++;
        }
    }
    min = String(min).padStart(2, '0');
    secs = String(secs).padStart(2, '0');
    timeString = min + ":" + secs;
    document.getElementById('measureTime').innerText = timeString;
}

function toggleAlarm(element) {
    const alarm = element.closest('.alarmObject');

    element.checked = !!element.checked;

    const alarmList = Array.from(document.querySelectorAll('#alarms .alarmObject'));

    const index = alarmList.indexOf(alarm);
    const hour = alarm.querySelector('.alarmHour').innerText.substring(0, 2);
    const minute = alarm.querySelector('.alarmHour').innerText.substring(3, 5);
    const alarmName = alarm.querySelector('.alarmName').innerText;
    const repeat = alarm.querySelector('.alarmRepeat').innerText;

    alarms[index].status = element.checked;

    let repeats = []

    if (repeat == "Every weekday") {
        for (let i = 1; i <= 5; i++) {
            repeats[i] = true;
        }
        repeats[0] = false;
        repeats[6] = false;
    }
    talk2ESP32("POST", "/alarms", alarms[index]);
    console.log(alarms);
}

function toggleAlarmView(mode) {
    let editor = document.getElementById('alarmEditor');
    let alarmsEl = document.getElementById('alarms');

    if (mode == 's') {
        editor.classList.add('show');
        alarmsEl.classList.add('show');
    }
    else if (mode == 'r') {
        editor.classList.remove('show');
        alarmsEl.classList.remove('show');
    }
}

function createAlarmObject(idx, name, timeH, timeM, aString, status = true) {
    document.getElementById('alarms').insertAdjacentHTML('beforeend', blankAlarm);

    const newAlarmObject = document.querySelectorAll('.alarmObject')[idx];

    newAlarmObject.children[0].innerText = name;
    newAlarmObject.children[1].children[0].innerText = timeH + ":" + timeM;
    newAlarmObject.children[1].children[1].querySelector('.alarmCheckbox').checked = status;
    newAlarmObject.children[2].childNodes[1].innerText = aString;

    toggleAlarm(document.querySelectorAll('.alarmObject .alarmCheckbox')[idx]);
    console.log(newAlarmObject);
}

function saveAlarm() {
    const alarmName = document.getElementById('getName').value || "New Alarm";
    const alarmTime = [];
    const index = Array.from(document.querySelectorAll("#alarms .alarmObject")).length;
    const repeatInputs = document.getElementById('getRepeat');

    let alarmString = "Once";
    let aux = 0;
    let repeats = [];

    document.querySelectorAll('.getTimeContainer').forEach((element, i) => {
        alarmTime[i] = element.children[1].innerText;
    });

    Array.from(repeatInputs.children).forEach((element, i) => {
        if (element.checked) {
            repeats[i] = true;
            if (aux === 0) {
                alarmString = "Every ";
                alarmString = alarmString + element.value;
            }
            else alarmString = alarmString + ", " + element.value;
            aux++;
        }
        else repeats[i] = false;
        if (aux === 6) alarmString = "Every day";
    });

    if (alarmString === "Every Monday, Tuesday, Wednesday, Thursday, Friday") alarmString = "Every weekday";
    else if (alarmString === "Every Sunday, Saturday") alarmString = "Every weekend";

    alarms[index] =
    {
        index: index,
        status: true,
        timeH: parseInt(alarmTime[0]),
        timeM: parseInt(alarmTime[1]),
        repeat: repeats,
        repeatS: alarmString,
        name: alarmName,
    };

    document.getElementById('getName').value = "";
    createAlarmObject(index, alarmName, alarmTime[0], alarmTime[1], alarmString);

    toggleAlarmView('r');
    console.log(alarms);
}

function insertRecent(duration, date, power, loadInsert = false) {
    const table = document.getElementById('recentTable').children[0];
    const rowIndex = table.children[0].childElementCount;

    const row = table.insertRow(rowIndex);

    const tableDuration = row.insertCell(0);
    const tableDate = row.insertCell(1);
    const tablePower = row.insertCell(2);

    tableDuration.innerText = duration;
    tableDate.innerText = date;
    tablePower.innerText = power + " W";
}

function eraseData(type) {
    if (type == 'alarms') alarms = [];
    else if (type == 'logs') grindData = [];

    talk2ESP32('POST', "/erase", { type: type });
}

function seconds2minutes(seconds, decimal = false) {
    const mins = Math.floor(seconds / 60);
    const secs = Math.round(seconds % 60);
    const hours = Math.floor(mins / 60);

    if (decimal) {
        if (mins > 0) return String(mins).padStart(2, '0') + "m " + String(secs.toFixed(2)).padStart(2, '0') + "s";
        else if (hours > 0) return String(hours).padStart(2, '0') + "h " + String(mins % 60).padStart(2, '0') + "m" + String(secs.toFixed(2)).padStart(2, '0') + "s";
        else return String(secs.toFixed(2)).padStart(2, '0') + "s";
    }
    else {
        if (mins > 0) return String(mins).padStart(2, '0') + "m " + String(secs).padStart(2, '0') + "s";
        else if (hours > 0) return String(hours).padStart(2, '0') + "h " + String(mins % 60).padStart(2, '0') + "m" + String(secs).padStart(2, '0') + "s";
        else return String(secs).padStart(2, '0') + "s";
    }

}

document.addEventListener('DOMContentLoaded', async () => {
    try {
        await loadSectionPartials();
    }
    catch (err) {
        console.log("Error while loading partials: ", err);
    }

    try {
        const data1 = await talk2ESP32("GET", "/alarms");
        const data2 = await talk2ESP32("GET", "/data");
        if (Array.isArray(data1)) alarms = data1;
        if (Array.isArray(data2)) grindData = data2;
    }
    catch (err) {
        console.log("Failed to load alarms", err);
    }

    document.getElementById('home').classList.add('active');
    display.innerHTML = sectionContent['home'];
    display.classList.add('show');
    loadContent('home');
});

sections.forEach(sec => {
    sec.addEventListener('click', () => {
        display.classList.remove('show');
        sections.forEach(j => j.classList.remove('active'));
        sec.classList.add('active');
        setTimeout(() => {
            switch (sec.id) {
                case 'home':
                    document.body.classList.add('moveHome');
                    document.body.classList.remove('moveSchedule');
                    document.body.classList.remove('moveInfo');
                    break;
                case 'schedule':
                    document.body.classList.remove('moveHome');
                    document.body.classList.add('moveSchedule');
                    document.body.classList.remove('moveInfo');
                    break;
                case 'info':
                    document.body.classList.remove('moveHome');
                    document.body.classList.remove('moveSchedule');
                    document.body.classList.add('moveInfo');
                    break;
            }
            display.classList.add('show');
            display.innerHTML = sectionContent[sec.id];
            loadContent(sec.id);
        }, 200);
    });
});

socket.onmessage = function (event) {
    let data = JSON.parse(event.data);
    let buttonState = data.state;
    let current = data.power;
    let registerGrind = data.register;
    let avgPower = data.avgPower;
    let measure = data.measure;

    let createNewAlarm = data.newAlarm;
    let alarmHour = data.hour;
    let alarmMinute = data.minute;

    let power = current * 3.7;

    if (measure) document.getElementById('measurePower').innerText = Number(power.toFixed(2)) + " W";
    if (measure) document.getElementById('measureCurrent').innerText = current + " A";

    if (buttonState) document.getElementById('onoffButton').click();

    if (registerGrind) {
        const today = new Date();

        const dur = document.getElementById('measureTime').textContent;
        const dat = String(today.getDate() + "/" + (today.getMonth() + 1));

        const table = document.getElementById('recentTable').children[0];
        const index = table.children[0].childElementCount;

        const newEntry =
        {
            duration: dur,
            date: dat,
            power: Number(avgPower*3.7).toFixed(2),
        };
        grindData.push(newEntry);
        insertRecent(newEntry.duration, newEntry.date, newEntry.power);
        talk2ESP32('POST', "/data", newEntry);
    }

    if (createNewAlarm) {
        const newIndex = alarms.length;
        alarms[newIndex] =
        {
            index: newIndex,
            status: true,
            timeH: parseInt(alarmHour),
            timeM: parseInt(alarmMinute),
            repeat: [false, false, false, false, false, false, false],
            repeatS: "Once",
            name: "New Alarm"
        }
        console.log(alarms);
        talk2ESP32("POST", "/alarms", alarms[newIndex]);

        const alarmsContainer = document.getElementById('alarms');
        if (alarmsContainer) createAlarmObject(newIndex, "New Alarm", String(alarmHour).padStart(2, '0'), String(alarmMinute).padStart(2, '0'), "Once");
        else console.log("Container not found. UI will be updated soon.");
    }
};

setInterval(() => {
    let hour = String(new Date().getUTCHours() - 3).padStart(2, '0');
    const minutes = String(new Date().getUTCMinutes()).padStart(2, '0');

    if (hour < 0) hour = String(Number(hour) + 24).padStart(2, '0');
    document.getElementById('hour').innerText = hour;
    document.getElementById('minute').innerText = minutes;
}, 1000);

socket.onopen = () => console.log("Web - WebSocket connected!");
socket.onclose = () => console.log("Web - WebSocket disconnected!");