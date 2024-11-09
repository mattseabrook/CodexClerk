let running = false;

const startStopButton = document.getElementById('startStopButton');
const filePathInput = document.getElementById('filePath');

startStopButton.addEventListener('click', () => {
    running = !running;
    startStopButton.innerText = running ? 'Stop' : 'Start';
    chrome.storage.local.set({ running, filePath: filePathInput.value }, () => {
        console.log(`Extension running: ${running}, File Path: ${filePathInput.value}`);
    });
});