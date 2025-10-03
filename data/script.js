const sections = document.querySelectorAll('.sections');
const getActive = document.getElementById('active');
const display = document.getElementById('display-container');
const homeButtons = document.querySelectorAll('.homeButton');
const initalContent = document.getElementById('startContent');
const template = document.getElementById('alarmObjectTemp');
const startData = 
{
    "status" : null,
    "thickness" : null
};

let alarms = [];

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
    `,
};

const blankAlarm = `
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
        <p class="alarmRepeat"></p>
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
        settings: '/html/settings.html'
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
    const aux = "http://192.168.0.2";
    const address = aux + route;

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
            document.getElementById('favoriteMethod').innerHTML = methods[method];
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
                    createAlarmObject(i, alarms[i].name, alarms[i].timeH, alarms[i].timeM, alarms[i].repeatS);
                }
            }

            console.log(alarms);
            break;
        default:
            break;
    }
}

function toggleStart()
{
    const thicknessSelect = document.getElementById("startContent").querySelector('.thickness');
    startData.status = "on";
    startData.thickness = thicknessSelect.value;

    console.log(startData);

    talk2ESP32("POST", "/on", startData);
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
        for(let i=0 ; i<=6 ; i++)
        {
            repeats[i] = false;
        }
    }

    alarms[index] = 
    {
        index : index,
        status : element.checked,
        timeH : hour,
        timeM : minute,
        repeat: repeats,
        repeatS: repeat,
        name : alarmName
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

function createAlarmObject(idx, name, timeH, timeM, aString)
{
    document.getElementById('alarms').insertAdjacentHTML('beforeend', blankAlarm);

    const newAlarmObject = document.querySelectorAll('.alarmObject')[idx]
    newAlarmObject.children[0].innerText = name;
    newAlarmObject.children[1].children[0].innerText = timeH + ":" + timeM;
    newAlarmObject.children[1].children[1].querySelector('.alarmCheckbox').checked = true;
    newAlarmObject.children[2].innerText = aString;
    toggleAlarm(document.querySelectorAll('.alarmObject .alarmCheckbox')[idx]);
}

function saveAlarm()
{
    const alarmName = document.getElementById('getName').value || "New Alarm";
    const alarmTime = [];
    const index = Array.from(document.querySelectorAll("#alarms .alarmObject")).length;
    const repeatInputs = document.getElementById('getRepeat');
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
        timeH : alarmTime[0],
        timeM : alarmTime[1],
        repeat: repeats,
        repeatS: alarmString,
        name : alarmName
    };

    document.getElementById('getName').value = "";
    createAlarmObject(index, alarmName, alarmTime[0], alarmTime[1], alarmString);
    
    toggleAlarmView('r');
    console.log(alarms);
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
        const data = await talk2ESP32("GET", "/alarms");
        if(Array.isArray(data)) alarms = data;
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