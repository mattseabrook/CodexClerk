const chatGPTTextArea = document.getElementById('prompt-textarea');

function parseSpecialCharacters() {
    if (chatGPTTextArea) {
        const lines = chatGPTTextArea.innerText.split('\n');
        for (let line of lines) {
            if (/^<<.*>>$/.test(line.trim())) {  // Check if the line contains "<< * >>"
                const filename = line.match(/^<<\s*(.*?)\s*>>$/)[1];
                console.log('Detected File Path:', filename);
                // Here you could trigger the runtime message to the background script
                chrome.runtime.sendMessage({ action: "fetchFileContent", filename: filename });
            }
        }
    }
}

const observer = new MutationObserver(() => {
    parseSpecialCharacters();
});

observer.observe(chatGPTTextArea, { childList: true, subtree: true });