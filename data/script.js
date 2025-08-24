const sections = document.querySelectorAll('.sections');
const getActive = document.getElementById('active');
const methodButton = document.getElementById('method');
const sectionContent = 
{
    home: `
    <div id="upText">
        <h2>Homepage</h2>
    </div>    
    <div id="sectionContent">
        <div id="powerSection">
                <h2 >Power</h2>
                <button>ON</button>
                <input class="slider" type="range" min="1" max="23">
                <h2>Saved profile</h2>
                <button>
                    <p id="method">Melitta</p>
                </button>
        </div>
        <div id="recentTable">
            <table>
                <tr>
                    <th>Date</th>
                    <th>Duration</th>
                    <th>Method</th>
                </tr>
                <tr>
                    <th>04. dez</th>
                    <th>2:40</th>
                    <th>Press</th>
                 </tr>
                 <tr>
                    <th>02. jan</th>
                    <th>1:50</th>
                    <th>V60</th>
                </tr>
            </table>
        </div>
    </div>
    `,
    schedule: `
        <h1>I am shakespeare in the flesh</h1>
    `,
    profiles: `
        <h1>Walt disney</h1>
    `,
    connection: `
        <h1>Nike</h1>
    `,
    settings: `
        <h1>Googoo</h1>
    `,
};

document.addEventListener('DOMContentLoaded', () =>
{
    document.getElementById('home').classList.add('active');
    document.getElementById('display-container').innerHTML = sectionContent['home'];
});

sections.forEach(sec =>
{
    sec.addEventListener('click', () => {
        sections.forEach(j => j.classList.remove('active'));
        sec.classList.add('active');
        document.getElementById('display-container').innerHTML = sectionContent[sec.id];
    });
});


/* Test function
document.getElementById('display-container').addEventListener('click', (e) => {
    if (e.target.id === 'method') {
        e.target.innerHTML = "Espresso";
    }
});
*/ 