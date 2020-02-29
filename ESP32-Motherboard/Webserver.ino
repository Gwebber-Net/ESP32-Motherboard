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

var bstate_none = new Image()
bstate_none.src ='assets/images/bstate_none.png';

var bstate_left = new Image()
bstate_left.src ='assets/images/bstate_left.png';

var bstate_right = new Image()
bstate_right.src ='assets/images/bstate_right.png';

var url_path = window.location.pathname;
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

var settings = null;
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

            settings = configuration;
            $.each(settings, function(key, value){
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

    $.each(settings, function(idx, setting){
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
    //console.log(summary_data);
    $.each(summary_data, function(key,value){
        //console.log(key, value);
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
        labels.push('pack'+(i+1));
    }    

    //'pack1', 'pack2', 'pack3', 'pack4', 'pack5', 'pack6']
    if ( ! pack_level_canvas ) {
        pack_level_canvas = new Chart(ctx_info, {
            type: 'bar',
            data: {
                labels: labels,
                datasets: [{
                    label: 'cell voltage',
                    data: [],
                    backgroundColor: 'rgba(54, 162, 235, 0.2)',
                    borderColor: 'rgba(54, 162, 235, 1)',
                    borderWidth: 2,
                },
                {
                  //type: 'category',
                  label: 'balance state',
                  //pointRadius: [15, 15, 15, 15, 15],
                  //pointHoverRadius: 20,
                  //pointHitRadius: 20,
                  pointStyle: [],
                  pointBackgroundColor: 'rgba(0,191,255)',
                  data: [],
                  type: 'line'
                }
                ]
            },
            options: {
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
    var pack_values = [];
    var pack_bstates = [];
    var pack_total_voltage = 0;
    var previous_bstate = 0;
    $.each(pack_data, function(key, value) {
        pack_values.push(value.voltage);
        pack_total_voltage = pack_total_voltage+value.voltage;
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

    var pack_size = $(pack_values).length;
    if ( pack_size != pack_level_canvas.data.datasets[0].data.length ) {
        // Pack size changed
        pack_level_canvas.destroy();
        pack_level_canvas = null;
        setup_pack_info(pack_size);
    }

    pack_level_canvas.data.datasets[0].data = pack_values;
    pack_level_canvas.data.datasets[1].data = pack_values;
    pack_level_canvas.data.datasets[1].pointStyle = pack_bstates;
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

void ReceiveSettings()
{
    String input = server.arg("plain");
    Serial.println(input);

    StaticJsonDocument<200> doc;
    DeserializationError err = deserializeJson(doc, input);
    if (err) 
    {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(err.c_str());
    }
    else
    {
        String s = doc["setting"];
        Serial.println(s);
    }  
}

void SendPackInfo()
{
  
  moduleVoltages[0][0] = (float)3.0;
  moduleVoltages[0][1] = (float)3.1;
  moduleVoltages[0][2] = (float)3.2;
  moduleVoltages[0][3] = (float)3.3;
  moduleVoltages[0][4] = (float)3.4;
  moduleVoltages[0][5] = (float)3.5;
  moduleVoltages[0][6] = (float)3.6;
  moduleVoltages[0][7] = (float)3.7;
  moduleVoltages[1][1] = (float)3.8;
  moduleVoltages[1][2] = (float)3.9;
  moduleVoltages[1][3] = (float)3.9;
  moduleVoltages[1][4] = (float)3.8;
  moduleVoltages[1][5] = (float)3.7;
  moduleVoltages[1][6] = (float)3.6;
  moduleVoltages[1][7] = (float)3.5;
  moduleVoltages[2][1] = (float)3.4;
  moduleVoltages[2][2] = (float)3.3;
  moduleVoltages[2][3] = (float)3.2;
  moduleVoltages[2][4] = (float)3.1;
  moduleVoltages[2][5] = (float)3.0;
  moduleVoltages[2][6] = (float)2.9;
  moduleVoltages[2][7] = (float)2.8;
  moduleVoltages[3][1] = (float)2.7;
  cellCount = 23;
  StaticJsonDocument<2000> doc;
  JsonArray array = doc.to<JsonArray>(); // Convert the document to an array.
  
    JsonObject arr; arr = array.createNestedObject(); // Create a Nested Object  
    arr["cell"] = 0;
    arr["voltage"] = moduleVoltages[0][0];
    byte counter = 1;;
    for(int l = 0; l < 10; l++)
    {
      for(int k = 1; k < 8; k++)
      {
        JsonObject arr = array.createNestedObject(); // Create a Nested Object  
        arr["cell"] = counter;
        arr["voltage"] = moduleVoltages[l][k];
        counter++;
        if(counter == cellCount)
        {
          break;
        }
      }
      if(counter == cellCount)
      {
        break;
      }
    }
    
  for(int i = 0; i < cellCount; i++)
  {
    

  }

  String output;
  serializeJson(doc, output);
  server.send(200,"application/json",output); 
}


// void SendPackInfo()
// {
//     String output = PackInfo();
//     server.send(200,"application/json",output);  
// }

void SendSummary()
{
    String summary = Summary();
    server.send(200,"application/json",summary);
}

void SendConfig()
{
    String settings = Settings();
    server.send(200,"application/json",settings);
}

void handleNotFound(){
    server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void InitialiseServer()
{
    server.on("/",[](){
        Serial.println("Service: /index.html");
        server.send_P(200, "text/html", webpage);}
    );
    server.on("/js/nrg.js",[](){
        Serial.println("Service: /js/nrg.js");
        server.send_P(200, "application/x-javascript", www_js_nrg);}
    );
    server.on("/css/nrg.css",[](){
        Serial.println("Service: /css/nrg.css");
        server.send_P(200, "text/css", www_css);
    });  
    //server.on("/voltage", server.send(200,"application/json", SendVoltage()));
    //server.on("/voltage", SendVoltage);
    server.on("/settings", ReceiveSettings);
    server.on("/api/packinfo", SendPackInfo);
    server.on("/api/config", SendConfig);
    server.on("/api/summary", [](){
        Serial.println("Service: /api/summary");
        SendSummary();
    });

    server.onNotFound(handleNotFound);
    server.begin();
}