#include <ArduinoJson.h>

char webpage[] PROGMEM = R"=====(
<html>
<head>
    <meta content="text/html;charset=utf-8" http-equiv="Content-Type" />
    <meta content="utf-8" http-equiv="encoding" />
    <link href='/css/nrg.css' rel='stylesheet' type='text/css' />

    <script src='https://code.jquery.com/jquery-3.4.1.min.js' integrity='sha256-CSXorXvZcTkaix6Yvo6HppcZGetbYMGWSFlBw8HfCJo=' crossorigin='anonymous'></script>
    <script src='https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.9.3/Chart.bundle.min.js'></script>
    <script type="text/javascript" src="js/nrg.js"></script>
</head>

<body>
    <div style='width: 100%; height: 100%; display: table;'>
        <div id='menu'>
            <div style='display: table-row; width:100%'>
                <div class='summary'> Pack Voltage (V)<div id='summary_value voltage' class='summary_value summary_voltage'>-</div> </div>
                <div class='summary'> Current (A) <div id='summary_value current' class='summary_value summary_current'>-</div></div>
                <div class='summary'> Lowest Cell  <div id='summary_value lowest_cell' class='summary_value summary_lowestcell'>-</div></div>
                <div class='summary'> Highest Cell <div id='summary_value highest_cell' class='summary_value summary_highestcell'>-</div></div>
                <div class='summary'> spare box <div id='summary_value spare1' class='summary_value'></div></div>
                <div class='summary'> [*] <div id='summary_value spare2' class='summary_value'></div></div>
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
setting['pack_label'] = "pack";
setting['pack_voltage_label'] = "Pack Voltage";
setting['pack_bgcolor_low'] = "rgba(229,12,12,0.7)";
setting['pack_bgcolor_norm'] = "rgba(12,12,229,0.7)";
setting['pack_bdcolor_low'] = "rgba(54, 162, 235, 1)";
setting['pack_bdcolor_norm'] = "rgba(54, 162, 235, 1)";

var bstate_none = new Image()
bstate_none.src ='assets/images/bstate_none.png';

var bstate_left = new Image()
bstate_left.src ='assets/images/bstate_left.png';

var bstate_right = new Image()
bstate_right.src ='assets/images/bstate_right.png';

var url_path = window.location.href;
//DEBUG
var url_path = '';

var bstate_bidirectional = new Image();
bstate_bidirectional.src ='assets/images/bstate_both.png';

var data
var pack_level_canvas = null;
Chart.defaults.global.elements.line.fill = false;

var last_status = { status:'Offline', message:'starting up', code: 200 };;
var connection = { status:'Offline', message:'starting up', code: 200 };

var int_summary = null;
var int_pack_info = null;

var global_summary;

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
    //console.log('STATUS ',connection.status, last_status.status);

    // State changed
    if ( connection.status != last_status.status ) {
        clearInterval(int_summary);
        clearInterval(int_pack_info);
        if ( connection.status == 'Online' ) {
            get_summary();
            get_pack_info();
            console.log('Offline to Online recovery');

            // Go online
            int_summary = setInterval(function(){ get_summary() }, global_interval);
            int_pack_info = setInterval(function(){ get_pack_info() }, global_interval);            
        }
    } else if ( connection.status == 'Offline' ) {
        console.log('main_interval: Attempting reconnect');
        get_config(function(){
            get_summary();
            get_pack_info();
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
            $.each(global_settings, function(key, value){
                console.log(key, value);

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
    $.each(summary_data, function(key,value){
        $('.summary_'+key).text(value);
    });
}

function get_summary() {
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/summary',
        dataType: 'json',
        success: function(pack_data) {
            global_summary = pack_data;
            update_main_summary(pack_data)
        },
        error: function(xhr, ajaxOptions, thrownError) {
            console.log( 'get_summary: ' + thrownError+' '+ xhr.statusText);
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            console.log( 'get_summary: ' + thrownError+' '+ xhr.statusText );
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        done: function() {
            console.log('done')
        }
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
                    label: setting['pack_voltage_label'],
                    data: [],
                    backgroundColor: [],
                    borderColor: [],
                    borderWidth: 1
                }],
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

    $.each(pack_data, function(key, value) {
        pack_values.push(value.voltage);
        pack_labels.push(setting['pack_label']+(key+1));
        if (typeof(global_summary) != "undefined" && key == global_summary.lowestcell) {
            pack_bgcolor.push(setting['pack_bgcolor_low']);
            pack_bdcolor.push(setting['pack_bdcolor_low']);
        } else {
            pack_bgcolor.push(setting['pack_bgcolor_norm']);
            pack_bdcolor.push(setting['pack_bdcolor_norm']);
        }

        switch( value.balance_state ) {
            case 0:
                if (previous_bstate == 2) {
                    pack_bstates.push(bstate_left);
                } else {
                     pack_bstates.push(bstate_none);
                }
                break;
            case 1:
                if (previous_bstate == 2) {
                    pack_bstates.push(bstate_bidirectional);
                } else {
                    pack_bstates.push(bstate_right);
                }
                break;
            case 2:
                if (previous_bstate == 2) {
                    pack_bstates.push(bstate_left);
                } else {
                    pack_bstates.push(bstate_none);
                }
                break;
            case 255:
                if (previous_bstate == 2) {
                    pack_bstates.push(bstate_left);
                } else {
                    pack_bstates.push(bstate_none);
                }
                break;
        }
        previous_bstate = value.balance_state;
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
    //TODO pack_level_canvas.data.datasets[1].data = pack_values;
    //TODO pack_level_canvas.data.datasets[1].pointStyle = pack_bstates;
    pack_level_canvas.update();
}

function get_pack_info() {
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/packinfo',
        dataType: 'json',
        success: function(pack_data) {
            update_pack_info(pack_data)
        },
        error: function(xhr, ajaxOptions, thrownError) {
            console.log( 'get_pack_info: ' + thrownError+' '+ xhr.statusText);
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            console.log( 'get_pack_info: ' + thrownError+' '+ xhr.statusText );
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        done: function() {
            console.log('done')
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

    get_summary();
    get_pack_info();

});
)=====";

char www_css[] PROGMEM = R"=====(
@import url(//db.onlinewebfonts.com/c/233131eb5b5e4a930cbdfb07008a09e1?family=LCD);
body {
    background-color: #101050;
    color: #f0f0f0
}
#menu {
    margin: 10px;
    padding: 10px;
    border: 5px solid #202080;
    font-family: "Verdana", Times, serif;
}
.summary {
    display: table-cell;
    width: 150px;
    text-align: center 
}
.summary_value {
    font-family: 'LCD',blank,arial;
    font-size: 80px;
    text-align: center;
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
    // Serial.println(settings);
    request->send(200, "application/json", settings);
  });
  server.on("/api/summary", HTTP_GET, [](AsyncWebServerRequest *request) {
    String summary = Summary();
    // Serial.println(settings);
    request->send(200, "application/json", summary);
  });
  server.on("/api/packinfo", HTTP_GET, [](AsyncWebServerRequest *request) {
    String packinfo = PackInfo();
    // Serial.println(settings);
    request->send(200, "application/json", packinfo);
  });

  server.begin();

  cellCount = 7;

  // Random generatar for the voltages and balance states
  randomSeed(analogRead(0));

  int rnd = random(0, 20);
  moduleVoltages[0][0] = 3.5 + (0.25 * rnd);
  for (int l = 0; l < 10; l++) {
    for (int k = 1; k < 8; k++) {
      int rnd = random(0, 20);
      moduleVoltages[l][k] = 3.5 + (0.25 * rnd);
    }
  }

  for (int l = 0; l < 10; l++) {
    moduleCellToDump[l] = 0;
    int rnd = random(0, 7);
    moduleCellToDump[l] = rnd;
  }

  for (int l = 0; l < 10; l++) {
    moduleCellToReceive[l] = 0;
    int rnd = random(0, 7);
    if (!moduleCellToDump[l]) {
      moduleCellToReceive[l] = rnd;
    }
  }
}
