const char app_script_js[] PROGMEM = R"rawliteral(
try {
    const conversions = [];
    const fields = document.querySelectorAll("#app_fields .fld");

    fields.forEach(fld => {
        const inputs = fld.querySelectorAll("input");
        const [minV, minT, maxV, maxT] = inputs;
        conversions.push({
            minV: parseFloat(minV.value),
            minT: parseInt(minT.value, 10),
            maxV: parseFloat(maxV.value),
            maxT: parseInt(maxT.value, 10)
        });
    });
    const mbflds = getFormValues("#mbflds input, #mbflds select");
    const payload = { conversions, ... mbflds };
    // console.log(mbflds);
    // console.log(payload);
    const res = await fetch('/app_fields', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
    });
    console.log(await res.text());
    if (!res.ok) throw new Error('Save failed');
    status.innerHTML = '<span class="ok">APP Settings saved.</span>';
} catch (e) {
    status.innerHTML = '<span class="err">' + e.message + '</span>';
}
)rawliteral";

const char rtuBridge_js[] PROGMEM = R"rawliteral(
function modbusBridgeFieldset(checkbox) {
    const fieldset = document.getElementById("modbusBridge");
    fieldset.disabled = !checkbox.checked;
}

function toggleMDBridgeFieldset() {
    const selector = document.getElementById("modbusModeSelector");
    const fieldset = document.getElementById("modbusBridge");
    if (fieldset) {
        fieldset.classList.toggle("hidden", selector.value === "rtu");
    }
}
)rawliteral";

const char setClientRTUValues[] PROGMEM = R"rawliteral(
    if (k === "clientRTUParity") {
        el.value = UART_Parity[v];
    } else if (k === "clientRTUStopBits") {
        el.value = UART_StopBits[v];
    } else {
        el.value = v;
    }
)rawliteral";

const char getClientRTUValues[] PROGMEM = R"rawliteral(
    } else if (el.name === "clientRTUParity") {
        data[el.name] = UART_ParityEnum[el.value];
    } else if (el.name === "clientRTUStopBits") {
        data[el.name] = UART_StopBitsEnum[el.value];
)rawliteral";

const char setModbusBridgeFieldset[] PROGMEM = R"rawliteral(
    var modbusBcb = document.getElementById('modbusBridgeCheckbox');
    if (modbusBcb) {
        modbusBridgeFieldset(document.getElementById("modbusBridgeCheckbox"));
    }
    toggleMDBridgeFieldset();
)rawliteral";