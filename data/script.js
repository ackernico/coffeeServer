const slides = document.querySelectorAll('.slide');
const getActive = document.getElementById('active');
const sectionContent = 
{
    power: `
        <h2>Homepage</h2>
        <h1>I am the number one rap of artist generation</h1>
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
    document.getElementById('power').classList.add('active');
    getActive.innerHTML = sectionContent['power'];
});

slides.forEach(slide =>
{
    slide.addEventListener('click', () => {
        slides.forEach(j => j.classList.remove('active'));
        slide.classList.add('active');
        getActive.innerHTML = sectionContent[slide.id];
    });
});