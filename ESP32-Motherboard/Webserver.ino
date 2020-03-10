#include <ArduinoJson.h>

char webpage[] PROGMEM = R"=====(
<html>
<head>
    <meta content="text/html;charset=utf-8" http-equiv="Content-Type" />
    <meta content="utf-8" http-equiv="encoding" />
    <link href='css/nrg.css' rel='stylesheet' type='text/css' />

    <script src='https://code.jquery.com/jquery-3.4.1.min.js' integrity='sha256-CSXorXvZcTkaix6Yvo6HppcZGetbYMGWSFlBw8HfCJo=' crossorigin='anonymous'></script>
    <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.3/Chart.bundle.min.js'></script>
    <script type="text/javascript" src="js/nrg.js"></script>
</head>

<body>
    <div style='width: 100%; height: 100%; display: table;'>
        <div id='menu'>
            <div class='summary_item hidden'><div class='summary_item label_key'>-</div><div class='summary_item label_value'>-</div> </div>
            <div class='summary_bar' style='display: table-row; width:100%'>
            </div>
        </div>
        <div class="chart-container" style="position: relative; height:50vh; width:95vw">
            <canvas id='pack_level' style='height: 602px; display: block'></canvas>
        </div>
    </div>
</body>
</html>
)=====";

char www_js_nrg[] PROGMEM = R"=====(
var global_interval = 5000;

// Label of pack chart
var setting = {};
var language = {};

// Chart X-axis label
language['pack_label'] = "pack";
// Chart Legend label
language['cell_voltage_label'] = "Pack Voltage";
language['balance_state_label'] = "Balance State";
// Summary label
language['voltage'] = "Pack Voltage";
language['current'] = "Pack Current";
language['lowestcell'] = "Lowest Cell";
language['highestcell'] = "Highest Cell";
language['soc'] = "State Of Charge";

setting['pack_bgcolor_receiving'] = 'rgba(0,102,0,1)';
setting['pack_bgcolor_dumping'] = 'rgba(255,204,51,1)';
setting['pack_bgcolor_none'] = 'rgba(0,0,153,1)';
setting['pack_bgcolor_passthrough'] = 'rgba(255,204,102,1)';

setting['pack_bdcolor_receiving'] = 'rgba(54, 162, 235, 1)';
setting['pack_bdcolor_dumping'] = 'rgba(162, 162, 235, 1)';
setting['pack_bdcolor_none'] = 'rgba(54, 162, 235, 1)';
setting['pack_bdcolor_passthrough'] = 'rgba(70, 112, 95, 1)';

var bstate_none = new Image()
bstate_none.src ='assets/images/bstate_none.png';
var bstate_left = new Image()
bstate_left.src ='assets/images/bstate_left.png';
var bstate_right = new Image()
bstate_right.src ='assets/images/bstate_right.png';

var url_path = '';

var bstate_bidirectional = new Image();
bstate_bidirectional.src ='assets/images/bstate_both.png';

var data
var pack_level_canvas = null;
Chart.defaults.global.elements.line.fill = false;

var last_status = { status:'Offline', message:'starting up', code: 200 };;
var connection = { status:'Offline', message:'starting up', code: 200 };

//var int_summary = null;
//var int_pack_info = null;
var int_alldata = null;

var global_settings = null;
var auth_uuid = null;

function unix2date(datetime) {
    var date = new Date(unix_timestamp*1000);
    // Hours part from the timestamp
    var hours = date.getHours();
    // Minutes part from the timestamp
    var minutes = '0' + date.getMinutes();
    // Seconds part from the timestamp
    var seconds = '0' + date.getSeconds();

    // Will display time in 10:30:23 format
    var formattedTime = hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);

    return formattedTime
}

function main_interval() {
    // State changed
    if ( connection.status != last_status.status ) {
        //clearInterval(int_summary);
        //clearInterval(int_pack_info);
        clearInterval(int_alldata);
        if ( connection.status == 'Online' ) {
            get_alldata();
            console.log('Offline to Online recovery');

            // Go online           
            int_alldata = setInterval(function(){ get_alldata() }, global_interval);    
        }
    } else if ( connection.status == 'Offline' ) {
        console.log('main_interval: Attempting reconnect');
        get_config(function(){
            get_alldata();
            console.log('main_interval: Offline recovery');
        });
    }
    last_status = connection;
}

// Main interval
setInterval(function(){ main_interval() }, global_interval);

function notify_alert(type, message, id=null) {
    if ( !id ) {
        var alert = $('.alert.template').clone().removeClass('template').addClass('alert-'+type);
        $(alert).text(message);
        $('#notifications').append($(alert).fadeIn());
    } else {
        var alert = $('#'+id);
        if ( $(alert).length == 0 ) {
            var alert = $('.alert.template').clone().removeClass('template').addClass('alert-'+type)
            $(alert).attr('id', id);
            $(alert).text(message);
            $('#notifications').append($(alert).fadeIn());
        } else {
            $(alert).text(message);
            $('#notifications').append($(alert).fadeIn());
        }
    }
}

function get_config(success) {
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/config',
        dataType: 'json',
        success: function(configuration) {
            box = $('.card-body.settings');
            tpl = $('.form-row.settings.template').clone().removeClass('template');
            $('#box_settings').find('.btn.btn-primary').attr('disabled', true);

            $(box).find('.form-row').remove();

            global_settings = configuration;
            $.each(global_settings, function(k, value){
                console.log(k, value);

                clone = $(tpl).clone();
                $(clone).find('.description').html(value.label+'<br/>'+value.description);
                if (value.type == 'int') {
                    type = 'number';
                } else if (value.type == 'float') {
                    type = 'number';
                    $(clone).find('.form-control').attr('pattern', '[0-9]+([\.,][0-9]+)?')
                    $(clone).find('.form-control').attr('step', '0.1')
                } else {
                    type = value.type;
                }
                $(clone).find('.form-control').attr('type', type);
                $(clone).find('.form-control').attr('id', value.setting);
                $(clone).find('.form-control').attr('min', value.min);
                $(clone).find('.form-control').attr('max', value.max);
                $(clone).find('.form-control').attr('value', value.value);
                $(clone).find('.invalid-feedback').html(
                    'Valid values from <span>'+value.min+'</span> to <span>'+value.max+'</span>.'
                );
                $(box).append(clone);
            });

            // Any errors should be removed
            $('#status_offline').fadeOut('normal', function() {
                $(this).remove();
            });
            connection = {status:'Online', message:'Service running', code: 200};
            if ( success ) {
                success()
            }
        },
        error: function(xhr, ajaxOptions, thrownError) {
            notify_alert('danger', 'Offline: Check service is running', 'status_offline');
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            notify_alert('danger', 'Offline: Check service is running', 'status_offline');
            console.log( xhr.status+ ' : '+thrownError+' '+ xhr.statusText );
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        done: function() {
            console.log('done')
        }
    });
}

function save_config() {
    conf_post = {}

    $.each(global_settings, function(idx, setting){
        var set_name = setting.setting;
        var set_value = $('#'+setting.setting).val();

        if (setting.type == 'int') {
            type = 'number';
            conf_post[set_name] = parseInt(set_value)
        } else if (setting.type == 'float') {
            type = 'number';
            conf_post[set_name] = parseFloat(set_value)
        } else {
            conf_post[set_name] = set_value;
        }
        console.log(conf_post)
    });

    $.ajax({
        method: 'POST',
        contentType: 'application/json',
        url: window.location.pathname+'api/config_save',
        data: JSON.stringify(conf_post),
        dataType: 'json',
        headers: {
            'Authorization-Token': auth_uuid,
        },
        success: function(configuration) {
            $('#exampleModalLong').find('.alert.alert-success').fadeIn('slow', function () {
                $(this).delay(5000).fadeOut('slow');
            });
        },
        error: function(xhr, ajaxOptions, thrownError) {
            if ( xhr.status >= 500 ) {
                notify_alert('danger', 'Offline: Check service is running', 'status_offline');

                connection = {status: 'Offline', message: 'Check system is up and on the network', code: xhr.status};

                $('#exampleModalLong').find('.modal-dialog .alert.alert-danger').fadeIn('slow', function () {
                    $(this).delay(5000).fadeOut('slow');
                });
            } else if ( xhr.status >= 401 ) {
                $('#exampleModalLong').find('.modal-dialog .alert.alert-danger').text('Authentication Required');
                $('#exampleModalLong').find('.modal-dialog .alert.alert-danger').fadeIn('slow', function () {
                    $(this).delay(5000).fadeOut('slow');
                });
            }
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            notify_alert('danger', 'Offline: Check service is running', 'status_offline');
            console.log( xhr.status+ ' : '+thrownError+' '+ xhr.statusText );

            connection = {status:'Offline', message: 'Check system is up and on the network', code: xhr.status};

            $('#exampleModalLong').find('.alert.alert-failed').fadeIn('slow', function () {
                $(this).delay(5000).fadeOut('slow');
            });
        },
        done: function() {
            console.log('done');
        }
    });
    return false;
}

function update_main_summary(summary_data) {
    // Clear all current data
    $('.summary_bar .summary_item').remove();
    // Popolate new data
    $.each(summary_data, function(idx){
        var tpl = $('.summary_item.hidden').clone().removeClass('hidden');
        $.each(summary_data[idx], function(k,v){
            // TODO use language
            $(tpl).find('.label_key').text(language[k]);
            $(tpl).find('.label_value').text(v);
        });
        $(".summary_bar").append(tpl);
    });
}

function setup_pack_info(size=0){
    labels = []
    for (i = 0; i < size; i++) {
        labels.push('pack'+(i));
    }

    //'pack1', 'pack2', 'pack3', 'pack4', 'pack5', 'pack6']
    if ( ! pack_level_canvas ) {
        pack_level_canvas = new Chart(ctx_info, {
            type: 'bar',
            data: {
                labels: [],
                datasets: [{
                    label: language['cell_voltage_label'],
                    data: [],
                    backgroundColor: [],
                    borderColor: [],
                    borderWidth: 1
                },
                {
                  label: language['balance_state_label'],
                  pointStyle: [],
                  pointBackgroundColor: 'rgba(0,191,255)',
                  data: [],
                  type: 'line'
                }]
            },
            options: {
                responsive: true,
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero: false
                        },
                        stacked: false
                    }],
                    xAxes: [
                        { stacked: false }
                    ]
                }
            }
        });
    } else {
        pack_level_canvas.labels = labels;
        pack_level_canvas.update();
    }
}

function update_pack_info(pack_data) {
    var pack_labels = [];
    var pack_values = [];
    var pack_bstates = [];
    var pack_bgcolor = [];
    var pack_bdcolor  = [];
    var pack_total_voltage = 0;
    var previous_bstate = 0;

    $.each(pack_data, function(k, value) {
        pack_values.push(value.voltage);
        pack_labels.push(language['pack_label']+k);
        pack_bgcolor.push(setting['pack_bgcolor_'+value['bstate']]);
        pack_bdcolor.push(setting['pack_bdcolor_'+value['bstate']]);
    });

    $('.summary_pack_voltage').text(pack_total_voltage.toFixed(2)+' Volts');
    
    // FIXME is this required, config should send that info
    var pack_size = $(pack_values).length;
    if ( pack_size != pack_level_canvas.data.datasets[0].data.length ) {
        // Pack size changed
        pack_level_canvas.destroy();
        pack_level_canvas = null;
        setup_pack_info(pack_size);
    }

    pack_level_canvas.data.labels = pack_labels;
    pack_level_canvas.data.datasets[0].data = pack_values;
    pack_level_canvas.data.datasets[0].backgroundColor = pack_bgcolor;
    pack_level_canvas.data.datasets[0].borderColor = pack_bdcolor;
    pack_level_canvas.data.datasets[1].data = pack_values;
    pack_level_canvas.data.datasets[1].pointStyle = pack_bstates;
    pack_level_canvas.update();
}

function get_alldata() {
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/alldata',
        dataType: 'json',
        success: function(alldata) {
            update_pack_info(alldata['packinfo']);            
            update_main_summary(alldata['summary'])
        },
        error: function(xhr, ajaxOptions, thrownError) {
            console.log( 'get_alldata: ' + thrownError+' '+ xhr.statusText);
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            console.log( 'get_alldata: ' + thrownError+' '+ xhr.statusText );
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        }
    });
}

$(document).ready(function(){
    ctx_info = $('#pack_level');

    // Configure charts
    setup_pack_info();

    // click close alerts
    $('body').on('click', '.alert', function(){
        $(this).fadeOut('normal', function() {
            $(this).remove();
        });
    });

    get_alldata();
});
)=====";

char www_css[] PROGMEM = R"=====(
@font-face {font-family: "LCD";
    src: url("//db.onlinewebfonts.com/t/233131eb5b5e4a930cbdfb07008a09e1.eot"); /* IE9*/
    src: url("//db.onlinewebfonts.com/t/233131eb5b5e4a930cbdfb07008a09e1.eot?#iefix") format("embedded-opentype"), /* IE6-IE8 */
    url("//db.onlinewebfonts.com/t/233131eb5b5e4a930cbdfb07008a09e1.woff2") format("woff2"), /* chrome firefox */
    url("//db.onlinewebfonts.com/t/233131eb5b5e4a930cbdfb07008a09e1.woff") format("woff"), /* chrome firefox */
    url("//db.onlinewebfonts.com/t/233131eb5b5e4a930cbdfb07008a09e1.ttf") format("truetype"), /* chrome firefox opera Safari, Android, iOS 4.2+*/
    url("//db.onlinewebfonts.com/t/233131eb5b5e4a930cbdfb07008a09e1.svg#LCD") format("svg"); /* iOS 4.1- */
}
body {
    background-color: #101050;
    color: #f0f0f0
}
#menu {
    margin: 5px;
    padding: 5px;
    height: 100px;
    border: 5px solid #202080;
    font-family: "Verdana", Times, serif;
}
.summary_bar {
    display: table-row;
    width:100%;
}
.label_key {
    float: left;
}
.summary {
    display: table-cell;
    width: 150px;
    text-align: center;
    float: left;
}
.summary_item {
    display: table-cell;
    width: 150px;
    text-align: center 
}
.summary_item.label_value {
    font-family: 'LCD',blank,arial;
    font-size: 80px;
    text-align: center;
}
.hidden {
    display: none;
}
)=====";

void InitialiseServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", webpage);
    });
    server.on("/js/nrg.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "application/x-javascript", www_js_nrg);
    });
    server.on("/css/nrg.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/css", www_css);
    });
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request) {
        String settings = Settings();
        request->send(200, "application/json", settings);
    });
    server.on("/api/summary", HTTP_GET, [](AsyncWebServerRequest *request) {
        String summary = Summary();
        request->send(200, "application/json", summary);
    });
    server.on("/api/packinfo", HTTP_GET, [](AsyncWebServerRequest *request) {
        String packinfo = PackInfo();
        request->send(200, "application/json", packinfo);
    });
    server.on("/api/alldata", HTTP_GET, [](AsyncWebServerRequest *request) {
        String alldata = AllData();
        request->send(200, "application/json", alldata);
    });
    server.begin();

    // TODO - move cellCount to summary or some config, move moduleVoltages randomness to comms where it should be (ie if not reading from modules, fake it) 
    cellCount = 7;
    
    GenerateRandomData();
}
