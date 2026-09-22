let checkStatusInterval;

function openDoor() {
  const button = document.getElementById("doorBtn");
  const label = document.getElementById("statusLabel");

  // Instantly update the UI
  button.disabled = true;
  label.innerText = "Opening...";
  label.style.color = "#ffc107";

  // Send request to the ESP32
  fetch("/open-door")
    .then(response => response.text())
    .then(data => {
      console.log("Server response:", data);

      // Start checking the door status
      checkStatusInterval = setInterval(pollStatus, 500);
    })
    .catch(error => {
      console.error("Error:", error);

      button.disabled = false;
      label.innerText = "Error!";
    });
}

function pollStatus() {
  const button = document.getElementById("doorBtn");
  const label = document.getElementById("statusLabel");

  // Get the current door status from the ESP32
  fetch("/get-status")
    .then(response => response.text())
    .then(status => {
      label.innerText = status;

      if (status === "Closed") {
        label.style.color = "#dc3545";
        button.disabled = false;
        clearInterval(checkStatusInterval);
      } else if (status === "Open") {
        label.style.color = "#28a745";
      }
    })
    .catch(error => {
      console.error("Error checking status:", error);
    });
}