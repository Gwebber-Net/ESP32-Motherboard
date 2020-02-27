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
var pack_history_canvas = null;
var pack_history_last_ts = null;
Chart.defaults.global.elements.line.fill = false;

var last_status = { status:'Offline', message:'starting up', code: 200 };;
var connection = { status:'Offline', message:'starting up', code: 200 };

var int_summary = null;
var int_pack_info = null;
var int_pack_history = null;

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
        clearInterval(int_pack_history);
        if ( connection.status == 'Online' ) {

            get_summary();
            get_pack_info();
            console.log('Offline to Online recovery');
            //load_pack_history();

            // Go online
            int_summary = setInterval(function(){ get_summary() }, global_interval);
            int_pack_info = setInterval(function(){ get_pack_info() }, global_interval);
            //int_pack_history = setInterval(function(){ update_pack_history(pack_history_last_ts) }, global_interval);
        }
    } else if ( connection.status == 'Offline' ) {
        console.log('main_interval: Attempting reconnect');
        get_config(function(){
            get_summary();
            get_pack_info();
            console.log('main_interval: Offline recovery');
            //load_pack_history();
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
        data: JSON.stringify({'request': 'config'}),
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
        data: JSON.stringify({'request': conf_post}),
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
        data: JSON.stringify({'request': 'summary'}),
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
    // var ctx_info = null;
    // var ctx_history = null;

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

function setup_pack_history_info(size=0) {
    var labels = []
    for (i = 0; i < size; i++) {
        labels.push((i+1));
    }
    // No history canvas
    if ( ctx_history.length == 0 ) {
        return;
    }
    //labels = ['one','two','three']
    //'pack1', 'pack2', 'pack3', 'pack4', 'pack5', 'pack6']
    if ( ! pack_history_canvas ) {
        pack_history_canvas = new Chart(ctx_history, {
            type: 'line',
            data: {
                labels: labels,
                datasets: []
            },
            options: {
                scales: {
                    yAxes: [{
                        ticks: {
                            beginAtZero: true
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
        pack_history_canvas.labels = labels;
        pack_history_canvas.update();
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

function load_history_canvas(pack_data) {
    // Store current timestamp used for updating canvas
    if ( typeof(pack_data.timestamp) == 'undefined') {
        console.log('FUCK, no TS');
    } else {
        pack_history_last_ts = pack_data.timestamp;
    }

    pack_history_canvas.data.labels = pack_data.labels;
    pack_history_canvas.data.datasets = pack_data.datasets;
    pack_history_canvas.update();
}

function update_history_canvas(pack_data) {
    // Store current timestamp used for updating canvas
    if ( typeof(pack_data.timestamp) == 'undefined') {
        console.log('FUCK, no TS');
    } else {
        if ( pack_data.timestamp > 0 ) {
            pack_history_last_ts = pack_data.timestamp;
        }
    }
    console.log('TODO update_history_canvas');

    for (i = 0; i < pack_data.labels.length; i++) {
        pack_history_canvas.data.labels.push(pack_data.labels[i]);
    }
    pack_history_canvas.data.labels.splice(0, 1);
    pack_history_canvas.data.datasets.forEach(function(dataset, index) {
        if ( typeof pack_data.datasets !== 'undefined' && typeof pack_data.datasets[index] !== 'undefined' ) {
            if ( typeof pack_data.datasets[index].data !== 'undefined' ) {
                for (i = 0; i < pack_data.datasets[index].data.length; i++) {
                    pack_history_canvas.data.datasets[index].data.push(pack_data.datasets[index].data[i]);
                }
            } else {
                pack_history_canvas.data.datasets[index].data.push(null);
            }
            pack_history_canvas.data.datasets[index].data.splice(0, 1);
        }
    });
    pack_history_canvas.update();
}

function get_pack_info() {
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/pack_info',
        data: JSON.stringify({'request': 'pack_info'}),
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

function load_pack_history() {
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/load_pack_history',
        data: JSON.stringify({'request': 'load_pack_history'}),
        dataType: 'json',
        success: function(pack_data) {
            // Load canvas
            load_history_canvas(pack_data)
        },
        error: function(xhr, ajaxOptions, thrownError) {
            console.log( thrownError+' '+ xhr.statusText);
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            console.log( thrownError+' '+ xhr.statusText );
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        done: function() {
            console.log('done')
        }
    });
}

function update_pack_history(start_timestamp) {
    console.log('update_pack_history TS:'+start_timestamp)
    $.ajax({
        method: 'GET',
        contentType: 'application/json',
        url: url_path+'api/update_pack_history',
        data: JSON.stringify({'request': 'update_pack_history', 'ts': start_timestamp}),
        dataType: 'json',
        success: function(pack_data) {
            // Update canvas
            update_history_canvas(pack_data)
        },
        error: function(xhr, ajaxOptions, thrownError) {
            console.log( thrownError+' '+ xhr.statusText);
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        fail: function(jqXHR, textStatus, errorThrown) {
            console.log( thrownError+' '+ xhr.statusText );
            connection = {status:'Offline', message:'Check system is up and on the network', code: xhr.status};
        },
        done: function() {
            console.log('done')
        }
    });
}

$(document).ready(function(){
    ctx_info = $('#pack_level');
    ctx_history = $('#pack_history');

    // Configure charts
    setup_pack_info();
    setup_pack_history_info();

    // click close alerts
    $('body').on('click', '.alert', function(){
        $(this).fadeOut('normal', function() {
            $(this).remove();
        });
    });

    $('.btn.btn-primary.logout').on('click', function() {
        auth_uuid = null
        $('.dropdown-menu').find('.btn.btn-primary.login').show();
        $('.dropdown-menu').find('.btn.btn-primary.logout').hide();
        $('#validationUsername').val('');
        $('#validationPassword').val('');
        $('.widget-heading.auth_name').text('Guest');
    });

    get_summary();
    get_pack_info();
    // First Load history
    //load_pack_history();

    $('.bg-midnight-bloom').on('click', function(item){
        notify_alert('danger', 'this won\'t work mate, give it up');
    });

    var forms = $('.needs-validation.settings');
    // Loop over them and prevent submission
    var validation = Array.prototype.filter.call(forms, function(form) {
        form.addEventListener('submit', function(event) {
            event.preventDefault();
            event.stopPropagation();

            var is_valid = true;
            if (form.checkValidity() === false) {
                is_valid = false;
            }
            form.classList.add('was-validated');
            if ( is_valid ) {
                save_config();
            }
        }, false);
    });

    var forms = $('.needs-validation.login');
    // Loop over them and prevent submission
    var validation = Array.prototype.filter.call(forms, function(form) {
        form.addEventListener('submit', function(event) {

            event.preventDefault();
            event.stopPropagation();
            if (form.checkValidity() === true) {
                var username = $('#validationUsername').val();
                var password = $('#validationPassword').val();
                $.ajax({
                    xhrFields: {
                        withCredentials: true
                    },
                    beforeSend: function (xhr) {
                        xhr.setRequestHeader('Authorization', 'Basic ' + btoa(username+':'+password));
                    },
                    method: 'POST',
                    contentType: 'application/json',
                    url: url_path+'api/authenticate',
                    success: function(auth_response) {
                        auth_uuid = auth_response;
                        $('.widget-heading.auth_name').text('Admin');
                        $('#modal_authenticate').find('.alert.alert-success').fadeIn('slow', function () {
                            $(this).delay(1000).fadeOut('slow', function() {
                                $('#modal_authenticate').find('.btn.btn-secondary').click();
                                $('.dropdown-menu').find('.btn.btn-primary.login').hide();
                                $('.dropdown-menu').find('.btn.btn-primary.logout').show();
                            });
                        });
                    },
                    error: function(xhr, ajaxOptions, thrownError) {
                        if ( xhr.status >= 500 ) {
                            notify_alert('danger', 'Offline: Check service is running', 'status_offline');

                            connection = {status: 'Offline', message: 'Check system is up and on the network', code: xhr.status};

                            $('.modal-dialog .alert.alert-danger').fadeIn('slow', function () {
                                $(this).delay(5000).fadeOut('slow');
                            });
                        } else if ( xhr.status >= 401 ) {
                            $('.modal-dialog .alert.alert-danger').text('Authentication Failed');
                            $('.modal-dialog .alert.alert-danger').fadeIn('slow', function () {
                                $(this).delay(5000).fadeOut('slow');
                            });
                        }
                    },
                    fail: function(jqXHR, textStatus, errorThrown) {
                        notify_alert('danger', 'Offline: Check service is running', 'status_offline');
                        console.log( xhr.status+ ' : '+thrownError+' '+ xhr.statusText );

                        connection = {status:'Offline', message: 'Check system is up and on the network', code: xhr.status};

                        $('.alert.alert-failed').fadeIn('slow', function () {
                            $(this).delay(5000).fadeOut('slow');
                        });
                    },
                    done: function() {
                        console.log('done')
                    }
                });
                form.classList.add('was-validated');

            }
        }, false);
    });

    $('.btn.set-default').on('click', function() {
        $.each(settings, function(key, value){
            console.log(key, value);

            item = $('#'+value.setting);

            $(item).val(value.defaultvalue);
        });
        $('#box_settings').find('.btn.btn-primary').attr('disabled', false);
    });

    $('.nav-item.tabs').on('click', function(item) {
        $(this).siblings().each(function(k, v){
            $(v).find('a').removeClass('active');
        });
        $(this).find('a').addClass('active');

        if ( item.target.text == 'History' ) {
            $('#tab_pack_level').hide();
            $('#tab_pack_history').show();
        } else {
            $('#tab_pack_level').show();
            $('#tab_pack_history').hide();
        }
    });
});
)=====";


char www_css[] PROGMEM = R"=====(
@import url(//db.onlinewebfonts.com/c/233131eb5b5e4a930cbdfb07008a09e1?family=LCD);
body {
    background-color: #909090;
}
#menu {
    margin: 10px;
    padding: 10px;
    border: 5px solid #858585;
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

void SendVoltage()
{

StaticJsonDocument<100> doc;

// create an object
//JsonObject object = doc.to<JsonObject>();
//object["voltage"] = 4.2;
doc["voltage"] = 4.2;
// serialize the object and send the result to Serial
String output;
serializeJson(doc, output);
server.send(200,"application/json",output);
}

void SendPackInfo()
{
  String output = PackInfo();
  server.send(200,"application/json",output);

  
}

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
  server.on("/voltage", SendVoltage);
  server.on("/settings", ReceiveSettings);
  server.on("/api/packinfo", SendPackInfo);
  server.on("/api/summary", SendSummary);

  server.on("/api/config",SendConfig);
  
  server.begin();

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

  moduleCellToDump[0] = 0;
  moduleCellToDump[1] = 0;
  moduleCellToDump[2] = 0;
  moduleCellToReceive[0] = 1;
  moduleCellToReceive[1] = 1;
  moduleCellToReceive[2] = 1;

  
}
