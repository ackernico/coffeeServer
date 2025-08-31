const sections = document.querySelectorAll('.sections');
const getActive = document.getElementById('active');
const display = document.getElementById('display-container');
const homeButtons = document.querySelectorAll('.homeButton');
const initalContent = document.getElementById('startContent');
const startData = 
{
    "status" : null,
    "thickness" : null
};
const sectionContent = 
{
    home: `
    <div id="upText">
        <h2>Homepage</h2>
    </div>    
    <div class="sectionContent">
        <div id="powerSection">
            <h2>Power</h2>
            <button class="homeButton" onclick="toggleStart()"> 
                <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAQAAADZc7J/AAAAIGNIUk0AAHomAACAhAAA+gAAAIDoAAB1MAAA6mAAADqYAAAXcJy6UTwAAAACYktHRAD/h4/MvwAAAAd0SU1FB+kHHRIlCvpI/V8AAAH7SURBVEjHnZWxSyNBGMXfjoukM+SCBItgkcpCYrOFxREOzuLqq/wbrIJYipVYWKW2POxsU5jjuPIQCVeKBIslWEhYUoaY8LsiyWYmye7mfNV837z35ptvdmalRJDjmpCQa3L6CLhlhtuPyEuMYoMRpSSeSXQoaSMebyjbgBJ7a1e3N6/In6by+qkhgTdaQ+7rh3w+e/15ytAE4NyiVbFRtWbOAWgy3z4nU9qA/SwD9hlMcyez1Db9mNjGTzPApx3n+mxPmljXVlzMgc5SG3Cmg3i8pbokNulZK42opVZQs74O6LEpjhziTWYPbpz8kVHNKnCsq8xTvNLYimpG1gGp7XWy9F5HbSusGlWs8I81jhxdlMCqGBWssGutFKoVBy0vXM1SIfkySce607vedafjZJKvSJ/iaMfZbU/fyUneYEFjsyKjFysMllo2WJK7rBejR3uKsjJA2TF4FIfOh9HINGg4/ENh6FqJIUGqPGBosbsYSVw4nmHyNigTOtyLSbpoXeeJRZCwuivvU5xNneJiSMOtgzINp3iAU0nyJAlfTX1dWHCsBz3oVdKOAgXWGz1BS9+sF5QCz/wPniksGLLL09ryJ3ZXNanI/Vry+7h5SxaGOlGqOKJO2gWUyHPJ20rxG5fkF/neShNfNX1RVRUVJEXq6K9+6feq/9Y/3oeew38Tm/MAAAAldEVYdGRhdGU6Y3JlYXRlADIwMjUtMDctMjlUMTg6Mzc6MTArMDA6MDB09MudAAAAJXRFWHRkYXRlOm1vZGlmeQAyMDI1LTA3LTI5VDE4OjM3OjEwKzAwOjAwBalzIQAAACh0RVh0ZGF0ZTp0aW1lc3RhbXAAMjAyNS0wNy0yOVQxODozNzoxMCswMDowMFK8Uv4AAAAASUVORK5CYII=">
                <p>ON</p>
            </button>
            <div id="startContent">
                <p>Grind thickness</p>
                <input id="thickness" type="range" min="0" max="22" step="1" name="thick">
                <h2>Saved profile</h2>
                <button class="homeButton" id="favoriteMethod"></button>
            </div>
        </div>
        <div id="recentTable">
            <table>
                <tr>
                    <th>Date</th>
                    <th id="duration">Duration</th>
                    <th>Method</th>
                </tr>
                <tr>
                    <td>04. dez</td>
                    <td>2:40</td>
                    <td>Melitta</td>
                 </tr>
                 <tr>
                    <td>02. jan</td>
                    <td>1:50</td>
                    <td>V60</td>
                </tr>
                <tr>
                    <td>04. dez</td>
                    <td>2:40</td>
                    <td>Press</td>
                 </tr>
                 <tr>
                    <td>02. jan</td>
                    <td>1:50</td>
                    <td>Espresso</td>
                </tr>
                <tr>
                    <td>04. dez</td>
                    <td>2:40</td>
                    <td>Press</td>
                 </tr>
                 <tr>
                    <td>02. jan</td>
                    <td>1:50</td>
                    <td>V60</td>
                </tr>
            </table>
        </div>
    </div>
    `,
    schedule: `
        <div id="upText">
            <h2>Schedule</h2>
        </div>
        <div class="sectionContent">
            <div id="alarmContainer">
                <button id="newAlarm">
                    <img class="sideIcons" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAAZElEQVRYR+2WSQoAMAgD6/8f3Q3sqVsKxct4ViMDRi09RK4xK7Maaju5YCXuwuoQDAABCEAAAp3AyV5Vf7/Nb7ZtUeLjdjAABMIJhK/h7c563okYLxkEIAABCHwnsLsdqnjrVQBAm4AJubvCowAAAABJRU5ErkJggg==">
                    <p>Add new alarm</p>
                </button>
            </div>
        </div>
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
const methods = 
{
    melitta: `
    <img src="data:image/png;base64,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">
    <p>Melitta</p>
    `,
    v60: `
    <img src="data:image/png;base64,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">
    <p>V60</p>
    `,
    espresso: `
    <img src="data:image/png;base64,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">
    <p>Espresso</p>
    `,
    french: `
    <img src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAYAAABzenr0AAAFkUlEQVRYR7WXS0xcVRjH7zAM8wJ5De9SAyZNJN0U0FURMNiFC5u0qJWujEk10Zh0YYJWW1YaI0h8WxOasDANdWWNsKCPxKYUY3BVWpEgJDxnGMgwvId5+Psuc8d53JnOpHKTM/fec77znf/3//7nu2cMSgZXQ0ODaXh4+BWr1XrOYDCUM3V8bW2tu6ysbCQDNzGmhkwmjo2NvepwOL6rqKgozMrKUgKBgOL3+8ezs7PPmM3m+5n40mzTBhAKhbLn5uYGSkpKTuXk5CgwoASDQWVnZ0eeO20226cHCkCcz8/PXyktLX2diNW1BMDe3h5EBN612+3fHggAIjcuLi4eKy4ufgnaT9PqhH65GJM0yDUCmOv0/2KxWCYyAZIyBVB+iByfJ79ni4qKyiRyoV6aBkBACBPYSf/U5ubm94z9AGBvOkCSAlhfXz+Kg6+JqpmoYhbWcyxApG1sbPh9Pt9V5r9XW1vrfBQIXQBut7uKiPptVluzKccUMhqN8X5YK6T2aWxoBrBhQJghp9PZn5+ff768vHwzFQhdAERRRhSNKFvGDZroohyFhHK5dMbUdLhcLh/MjSDajYwBPIq2/3PcAJUGVPwi92byLfLe55bI5UdTvAgtfIW0PnkP96cSs+YvhC3EuO5VVlb+Qep86iIsbIbuAe4nhU5NTFp+JcdavqP7onIeM65no80XsEtLS57d3d2rpPlSfX39soqcYtLMYD9iexLjSUTYS5TqNiKH6lpSdsNiFJbe4f1ZwF3D/mf6hUXVDl8iTAePHxKQlfFufC3QjKzxNO3M9va2g9ryeUdHx/sR6hDOKYwv0/KgqaeqquoSjvaVFncB8BoKfxmnnSyeUIKpBZX4GWEsF5vj1JG/xIWkGwBvcf9yYWFhGtAnIgBkkFL7Jot+hm02VF2Eot54EFIZJycnB2pqaqQqXmCRj+MBEuFhtuIdvhm5sNAc/aFifiP2t1h8jbkvxIiHQRPUdEF75+zs7AwOWmBiNnoBATA6OjrQ2NiYEgB5FgB2FhEA45qPMICbvEuKYwGIEQBa+OINbW1tuXEgVfAfPQCcDU7DzgWTyaTLQBoA1vD7XwqiELZCzyA0unGuC2BiYiKSggMDkIwBAep0un4qKMhvRwMfAOATPQ08NgOIaFnyRwpmYOMQx7A8ETLNCEM9qPkE418A4rIAoJZIW8nLy3Otrq4eJj13yH0qDaROAfvZhd8mCkYJjn6kOYhW3Ur0CxgTi29z3xIAMKaw/Sb4Cr6GbZADyt00ACSKkAVaiW7Q7w+4gsFA08rKioX3r4gWFmyKyaSehrTyKs8G+fh4vV4F2h/S3i4sLDQ9NgBhADAtUD9NTbADwpqbmxuf7sg7TEm13Kyurt4Op+C3qDoQsw3xe5MUJU+BMCAakMOIAEi6apIBKUSaCAHVhC4eaqYE1obf64y78d2W8BXTUoCTZapYS3wdSAeMxgBpeIKF2knf35IqrhqeP+K5jf4bQ0ND7akAeMIAMjpkCkA50BDAbc6RR9gZTtqeiJeUFDNsh4FpGD5XUFBwIwEAFLWCVFLgRfUtKDlCXzrRiw3/lorYDbc4jh1l8Vl87opQzWbLOr5/R8hXAPOnquB4pwKAvkEoEgBSx9UvWSbX1NRUvggNQT4FgJPMVX0gYh8APNG+EgBA3fMY/Yp4vOTruUzP+eKcD5mV220oPsL9OCJ8kCyABAB865+BrmH2so1J3URyFy1EjlWao/CxWPrlJBIM24SYL2oTBnoJxigs8l9yJm0ATLRwpL7IH4s3mCQVz8+2VNeTph3Dw3c5sgsIGVdBSq5FcOhgnd3QU1dX9w22+8clnUv3MIkDMyCO8be7kjkGIpGp6gJoQ4FaxePxqHetX+y0Pu4BdsJ0X1/f/a6urshpVg/Av0tlnCT16nJ5AAAAAElFTkSuQmCC">
    <p>French press</p>
    `,
    moka: `
    <img src="data:image/png;base64,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">
    <p>Moka pot</p>
    `,
};

const method = 'v60';

function loadFavorite(content)
{
    document.getElementById('favoriteMethod').innerHTML = methods[content];
}

function toggleStart()
{
    //Getting input slider tick
    const thicknessSelect = document.getElementById('thickness');
    startData.status = "on";
    startData.thickness = thicknessSelect.value;

    //Hiding initial content

    //HTTP request
    fetch("http://192.168.0.7/on", 
    {
        method : "POST",
        headers : {"Content-Type" : "application/json"},
        body : JSON.stringify(startData)
    })
        .then(res=> 
        {
            if(!res.ok) throw new Error(res.status);
            return res.text();
        })
        .then(data =>
        {
            console.log("ESP32 responded with: ", data);
        })
        .catch(err => 
        {
            console.log("Error during the process: ", err);
        });
}

document.addEventListener('DOMContentLoaded', () =>
{
    document.getElementById('home').classList.add('active');
    display.innerHTML = sectionContent['home'];
    display.classList.add('show');
    loadFavorite(method);
});

sections.forEach(sec =>
{
    sec.addEventListener('click', () => {
        display.classList.remove('show');
        sections.forEach(j => j.classList.remove('active'));
        sec.classList.add('active');
        setTimeout(() => 
        {
            display.classList.add('show');
            display.innerHTML = sectionContent[sec.id];
            loadFavorite(method);
        }, 200);
    });
});