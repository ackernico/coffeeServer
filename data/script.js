let socket = new WebSocket("ws://192.168.0.4/ws");
const ip = "http://192.168.0.4";

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

const sectionContent = [];
const methods = 
{
    melitta: `
    <img src="../assets/melitta.png">
    <p>Melitta</p>
    `,
    v60: `
    <img src="../assets/v60.png">
    <p>V60</p>
    `,
    espresso: `
    <img class="sideIcons" src="../assets/espresso.png">
    <p>Espresso</p>
    `,
    french: `
    <img src="../assets/french.png">    
    <p>French Press</p>
    `,
    moka: `
    <img src="../assets/moka.png">
    <p>Moka Pot</p>
    `
};

const methodsClicks = 
{
    melitta : null,
    v60 : null,
    espresso : null,
    french : null,
    moka : null
}   

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

async function loadSectionPartials()
{
    const files = 
    {
        home: '/html/home.html',
        schedule: '/html/schedule.html',
        profiles: '/html/profiles.html',
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

function adjustTime(arrow, timeReset, order)
{
    let timeElement = arrow.closest(".getTimeContainer").children[1];
    let time = Number(timeElement.innerText);
    let reset;
    
    if(timeReset == 'h') reset = 23;
    else if(timeReset == 'm') reset = 59;

    if(order == 's') time = time + 1;
    else if(order == 'm') time = time - 1;
    if(time > reset) time = 0;
    if(time < 0) time = reset;

    timeElement.innerText = String(time).padStart(2, '0');
}

async function talk2ESP32(method, route, jsonData)
{
    const address = ip + route;

    const labels = 
    {
        method : method.toUpperCase(),
        headers : {"Content-Type" : "application/json"},
    }

    if(labels.method !== "GET" && labels.method !== "HEAD" && jsonData !== undefined)
    {
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

function loadContent(content)
{
    switch(content)
    {
        case 'home':
            display.innerHTML = sectionContent['home'];
            document.getElementById('favoriteMethod').innerHTML = methods[method];       
            if(grindState)
            {
                document.getElementById('startContent').classList.remove('show');
                document.getElementById('grindMeasurements').classList.add('show');
                document.getElementById('measurementLabel').classList.add('show');
                document.getElementById('buttonLabel').innerText = "OFF";
            }
            else
            {
                document.getElementById('startContent').classList.add('show');
                document.getElementById('grindMeasurements').classList.remove('show');
                document.getElementById('measurementLabel').classList.remove('show');
                document.getElementById('buttonLabel').innerText = "ON";
            }

            for (let i = 0; i < grindData.length; i++) 
            {
                insertRecent(grindData[i].duration, grindData[i].date, grindData[i].thickness, grindData[i].power);
                console.log(i);
            }     
            console.log(grindData);
            break;
        case 'schedule':
            const checkbox = document.querySelectorAll('.alarmObject .alarmCheckbox');   

            document.getElementById('getTime').querySelectorAll('.getTimeButton').forEach((element) =>
            {
                element.innerText = "00";
            });

            for(let i=0 ; i<=alarms.length ; i++)
            {
                if(alarms[i] != undefined)
                {
                    createAlarmObject(i, alarms[i].name, String(alarms[i].timeH).padStart('2', 0), String(alarms[i].timeM).padStart('2', 0), alarms[i].repeatS, alarms[i].thickness);
                }
            }
            console.log(alarms);
            break;
        default:
            break;
    }
}

function toggleStart(isFavorite) 
{
    let startData =
    {
        "status" : null,
        "thickness" : null
    };

    grindState = !grindState;
    timer = grindState;

    if(grindState)
    {
        startData.status = "on";
        secs = -1;
        min = 0;
        grindTimer();
        document.getElementById('startContent').classList.remove('show');
        document.getElementById('grindMeasurements').classList.add('show');
        document.getElementById('measurementLabel').classList.add('show');
        document.getElementById('buttonLabel').innerText = "OFF";
    }
    else
    {
        startData.status = "off";
        document.getElementById('startContent').classList.add('show');
        document.getElementById('grindMeasurements').classList.remove('show');
        document.getElementById('measurementLabel').classList.remove('show');
        document.getElementById('buttonLabel').innerText = "ON";
        clearTimeout(timerID);
        timerID = null;
    }

    startData.thickness = document.querySelector('#startContent .thickness').value;

    if(!isFavorite) document.getElementById('measureThickness').innerText = startData.thickness + " clicks";
    else if(isFavorite) document.getElementById('measureThickness').innerText = String(method);
    talk2ESP32("POST", "/on", startData);
}

function grindTimer()
{
    timerID = setTimeout(grindTimer, 1000);
    if(timer)
    {
        secs++;
        if(secs >= 60)
        {
            secs = 0;
            min++;
        }
    }
    min = String(min).padStart(2, '0');
    secs = String(secs).padStart(2, '0');
    timeString = min + ":" + secs;
    document.getElementById('measureTime').innerText = timeString;
}

function toggleAlarm(element)
{
    const alarm = element.closest('.alarmObject');

    const alarmList = Array.from(document.querySelectorAll('#alarms .alarmObject'));

    const index = alarmList.indexOf(alarm);
    const hour = alarm.querySelector('.alarmHour').innerText.substring(0, 2);
    const minute = alarm.querySelector('.alarmHour').innerText.substring(3, 5);
    const alarmName = alarm.querySelector('.alarmName').innerText;
    const repeat = alarm.querySelector('.alarmRepeat').innerText;
    const thickness  = alarm.querySelector('.alarmThickness').innerText;

    let repeats = []

    if(repeat == "Every weekday")
    {
        for(let i=1 ; i<=5 ; i++)
        {
            repeats[i] = true;
        }
        repeats[0] = false;
        repeats[6] = false;
    } 
    else if(repeat == "No repeat")
    {

        thickness : parseInt(thickness.match(/\d+/g))
    };
    talk2ESP32("POST", "/alarms", alarms[index]);
}

function toggleAlarmView(mode)
{
    let editor = document.getElementById('alarmEditor');
    let alarmsEl = document.getElementById('alarms');

    if(mode == 's')
    {
        editor.classList.add('show');
        alarmsEl.classList.add('show');
    }
    else if(mode == 'r')
    {
        editor.classList.remove('show');
        alarmsEl.classList.remove('show');
    }
}

function createAlarmObject(idx, name, timeH, timeM, aString, thick)
{
    document.getElementById('alarms').insertAdjacentHTML('beforeend', blankAlarm);

    const newAlarmObject = document.querySelectorAll('.alarmObject')[idx];

    newAlarmObject.children[0].innerText = name;
    newAlarmObject.children[1].children[0].innerText = timeH + ":" + timeM;
    newAlarmObject.children[1].children[1].querySelector('.alarmCheckbox').checked = true;
    newAlarmObject.children[2].childNodes[1].innerText = aString;
    newAlarmObject.children[2].childNodes[3].innerText = thick + " clicks";

    document.querySelectorAll('.alarmObjectContainer').forEach((obj, i) =>
    {
        const edit = obj.querySelector('.editMode');
        obj.addEventListener('mouseenter', () =>
        {
            edit.classList.add('show');
        });
        obj.addEventListener('mouseleave', () =>
        {
            edit.classList.remove('show');
        });

        edit.addEventListener('click', async () =>
        {
            obj.remove();
            alarms.splice(i, 1);
            talk2ESP32("POST", "/alarms", alarms);
        });
    });

    toggleAlarm(document.querySelectorAll('.alarmObject .alarmCheckbox')[idx]);
    console.log(newAlarmObject);
}

function saveAlarm(editMode)
{
    const alarmName = document.getElementById('getName').value || "New Alarm";
    const alarmTime = [];
    const index = Array.from(document.querySelectorAll("#alarms .alarmObject")).length;
    const repeatInputs = document.getElementById('getRepeat');
    const alarmThickness = document.querySelector('.thickness').value;
    
    let alarmString = "Once";
    let aux = 0;
    let repeats = [];

    document.querySelectorAll('.getTimeContainer').forEach((element, i) =>
    {
        alarmTime[i] = element.children[1].innerText;
    });

    Array.from(repeatInputs.children).forEach((element, i) =>
    {
        if(element.checked)
        {
            repeats[i] = true;
            if(aux === 0)
            {
                alarmString = "Every ";
                alarmString = alarmString + element.value;
            }
            else alarmString = alarmString + ", " + element.value;
            aux++;
        } 
        else repeats[i] = false;
        if(aux === 6) alarmString = "Every day";
    });

    if(alarmString === "Every Monday, Tuesday, Wednesday, Thursday, Friday") alarmString = "Every weekday";
    else if(alarmString === "Every Sunday, Saturday") alarmString = "Every weekend";

    alarms[index] = 
    {
        index : index,
        status : true,
        timeH : parseInt(alarmTime[0]),
        timeM : parseInt(alarmTime[1]),
        repeat: repeats,
        repeatS: alarmString,
        name : alarmName,
        thickness : parseInt(alarmThickness.match(/\d+/g))
    };

    document.getElementById('getName').value = "";
    createAlarmObject(index, alarmName, alarmTime[0], alarmTime[1], alarmString, parseInt(alarmThickness.match(/\d+/g)));
    
    toggleAlarmView('r');
    console.log(alarms);
}

function insertRecent(duration, date, thickness, power, loadInsert = false)
{
    const table = document.getElementById('recentTable').children[0];
    const rowIndex = table.children[0].childElementCount;

    const row = table.insertRow(rowIndex);

    const tableDuration = row.insertCell(0);
    const tableDate = row.insertCell(1);
    const tableThickness = row.insertCell(2);
    const tablePower = row.insertCell(3);

    tableDuration.innerText = duration;
    tableDate.innerText = date;
    tableThickness.innerText = thickness;
    tablePower.innerText = power;
}

document.addEventListener('DOMContentLoaded', async () =>
{
    try
    {
        await loadSectionPartials();
    }
    catch(err)
    {
        console.log("Error while loading partials: ", err);
    }
    
    try
    {
        const data1 = await talk2ESP32("GET", "/alarms");
        const data2 = await talk2ESP32("GET", "/data");
        if(Array.isArray(data1)) alarms = data1;
        if(Array.isArray(data2)) grindData = data2;
    }
    catch(err)
    {
        console.log("Failed to load alarms", err);
    }

    document.getElementById('home').classList.add('active');
    display.innerHTML = sectionContent['home'];
    display.classList.add('show');
    loadContent('home');
});

sections.forEach(sec =>
{
    sec.addEventListener('click', () => 
    {
        display.classList.remove('show');
        sections.forEach(j => j.classList.remove('active'));
        sec.classList.add('active');
        setTimeout(() => 
        {
            display.classList.add('show');
            display.innerHTML = sectionContent[sec.id];
            loadContent(sec.id);
        }, 200);
    });
});

socket.onmessage = function(event)
{
    let data = JSON.parse(event.data);
    let buttonState = data.state;
    let power = data.power;
    let registerGrind = data.register;
    let avgPower = data.avgPower;

    document.getElementById('measurePower').innerText = power + " W";

    if(buttonState) document.getElementById('onoffButton').click();

    if(registerGrind)
    {
        const today = new Date();

        const dur = document.getElementById('measureTime').textContent;
        const dat = String(today.getDate() + "/" + (today.getMonth() + 1));
        const thic = document.getElementById('measureThickness').textContent;

        const table = document.getElementById('recentTable').children[0];
        const index = table.children[0].childElementCount;

        const newEntry = 
        {
            duration : dur,
            date : dat,
            thickness : thic,
            power : avgPower
        };
        grindData.push(newEntry);
        insertRecent(newEntry.duration, newEntry.date, newEntry.thickness, newEntry.power);
        talk2ESP32('POST', "/data", newEntry);
    }
};

socket.onopen = () => console.log("Web - WebSocket connected!");
socket.onclose = () => console.log("Web - WebSocket disconnected!");