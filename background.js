chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
    if (request.action === "fetchFileContent") {
        const { filename } = request;
        fetch(`http://localhost:3000/getFile?filename=${encodeURIComponent(filename)}`)
            .then(response => response.text())
            .then(data => {
                sendResponse({ content: data });
            })
            .catch(error => {
                console.error('Error fetching file content:', error);
                sendResponse({ error: error.toString() });
            });
        return true;
    }
});