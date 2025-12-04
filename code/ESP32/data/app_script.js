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
    const mbflds = getFormValues("#mbflds input");
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
