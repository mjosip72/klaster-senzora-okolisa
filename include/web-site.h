
#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html><html lang="hr"><head><meta charset="UTF-8"><title>Klaster senzora okoliša</title><style>:root{--theme-accent:#007E59;--theme-background:#1E1E1E;--theme-background-alt:#333333;--theme-background-alt2:#272727;font-family:'Segoe UI',Tahoma,Verdana,sans-serif;background-color:var(--theme-background);color:white;}::selection{color:white;background:var(--theme-accent);}::-webkit-scrollbar{width:8px;height:8px;}::-webkit-scrollbar-track{background:var(--theme-background);}::-webkit-scrollbar-thumb{background:var(--theme-background-alt);border-radius:4px;}::-webkit-scrollbar-thumb:hover{background:var(--theme-accent);}::-webkit-scrollbar-corner{appearance:none;}body{margin:auto;}.title{position:relative;text-align:center;font-size:40px;padding:20px 0px;background-color:var(--theme-accent);margin-bottom:40px;user-select:none;}.home-content{text-align:center;}.chart{display:inline-block;margin:20px;padding:40px 10px 10px 10px;background-color:#161616;border-radius:40px;}.menu-btn{display:inline-block;position:absolute;top:24px;right:20px;padding:0 12px;}.menu-icon{width:32px;height:4px;background-color:white;margin:8px 0;border-radius:2px;}.dropdown-menu{opacity:0;pointer-events:none;position:absolute;top:100%;right:0;text-align:start;transform:scale(0,0);transition:opacity 60ms ease-in-out,transform 60ms ease-in-out;}.dropdown-menu.active{opacity:1;pointer-events:auto;transform:scale(1,1);}.dropdown-menu-item{padding:4px 60px 4px 12px;background-color:var(--theme-background-alt);}.dropdown-menu-item.active{background-color:var(--theme-accent);}.lcd-content{display:inline-block;margin-left:40px;font-size:36px;}button{font-family:'Segoe UI',Tahoma,Verdana,sans-serif;font-size:36px;padding:6px 16px;cursor:pointer;outline:0;margin:18px 20px;text-decoration:none;border-radius:32px;background-color:transparent;border:4px solid var(--theme-accent);color:white;-webkit-tap-highlight-color:#007e585d;}textarea{background-color:var(--theme-background-alt2);color:white;font-family:'Segoe UI',Tahoma,Verdana,sans-serif;font-size:36px;outline:0;border:none;resize:none;width:32ch;height:5.4em;border-bottom:4px solid var(--theme-background-alt2);transition-duration:0.4s;}textarea:focus{border-bottom:3px solid var(--theme-accent);}p{font-family:consolas;white-space:pre;width:20ch;user-select:none;padding:1px 6px;margin:0;color:white;background-color:var(--theme-background-alt2);}</style></head><body><div class="title"><span>Klaster senzora okoliša</span><div class="menu-btn" onclick="on_menu_btn()" data-dropdown><div class="menu-icon" data-dropdown></div><div class="menu-icon" data-dropdown></div><div class="menu-icon" data-dropdown></div></div><div class="dropdown-menu" data-dropdown><div class="dropdown-menu-item" data-dropdown onclick="on_menu_item(0)">Početna</div><div class="dropdown-menu-item" data-dropdown onclick="on_menu_item(1)">LCD</div><div class="dropdown-menu-item" data-dropdown onclick="on_menu_item(2)">RTC</div></div></div><div main-page class="home-content"><div><div class="chart"><canvas id="temp" width="360" height="360"></canvas></div><div class="chart"><canvas id="press" width="360" height="360"></canvas></div></div><div><div class="chart"><canvas id="hum" width="360" height="360"></canvas></div><div class="chart"><canvas id="iaq" width="360" height="360"></canvas></div></div></div><div main-page><div class="lcd-content"><div style="margin-bottom:12px;">Unesite tekst za prikaz na LCD ekranu</div><textarea spellcheck="false" placeholder="Tekst..."></textarea><div style="text-align:center;"><button onclick="on_btn_clear()">Očisti</button><button onclick="on_btn_show()">Prikaži</button></div></div><br><div class="lcd-content"><div style="margin-bottom:12px;">Pregled</div><p style="padding-top:4px;"></p><p></p><p></p><p style="padding-bottom:4px;"></p></div><div class="lcd-content"><div style="margin-bottom:12px;">Pozadinsko osvjetljenje</div><div style="text-align:center;"><button onclick="on_btn_backl('on')">On</button><button onclick="on_btn_backl('off')">Off</button><button onclick="on_btn_backl('blink')">Treptanje</button></div></div></div><div main-page><button style="margin-left:40px;" onclick="set_time()">Sinkroniziraj vrijeme</button></div><script>let ws=new WebSocket("ws://esc.local/esc");ws.onmessage=e=>{let data=e.data.split(",");temp_data.value=data[0];press_data.value=data[1];hum_data.value=data[2];iaq_data.value=data[3];render_charts();};document.addEventListener("click",e=>{if(e.target.hasAttribute("data-dropdown"))return;if(is_menu_open){dropdown_menu.classList.remove("active");is_menu_open=false;}});let is_menu_open=false;let dropdown_menu=document.querySelector(".dropdown-menu");function on_menu_btn(){if(is_menu_open)dropdown_menu.classList.remove("active");else dropdown_menu.classList.add("active");is_menu_open=!is_menu_open;}let menu_items=document.querySelectorAll(".dropdown-menu-item");let main_pages=document.querySelectorAll("[main-page]");let current_main_page=0;function on_menu_item(id){if(current_main_page !=id)on_menu_btn();current_main_page=id;for(let i=0;i<menu_items.length;i++){menu_items[i].classList.remove("active");main_pages[i].style.display="none";}menu_items[id].classList.add("active");main_pages[id].style.display="";}on_menu_item(0);const chart_width=360;const chart_height=360;let temp_chart=document.querySelector("#temp").getContext("2d");let press_chart=document.querySelector("#press").getContext("2d");let hum_chart=document.querySelector("#hum").getContext("2d");let iaq_chart=document.querySelector("#iaq").getContext("2d");const temp_data={title:"Temperatura",suffix:" °C",min:-10,max:40,color:"#FF495E",value:0};const press_data={title:"Tlak zraka",suffix:" hPa",min:980,max:1020,color:"#00E29D",value:0};const hum_data={title:"Vlažnost",suffix:" %",min:0,max:100,color:"#008FF6",value:0};const iaq_data={title:"IAQ",suffix:"",min:0,max:350,color:"#FFB134",value:0};function render_chart(g,data){let yoff=60;let mx=chart_width/2;let my=chart_height/2+yoff;let r=chart_width/2-30;g.clearRect(0,0,chart_width,chart_height);g.strokeStyle="#333333";g.lineCap="round";g.lineWidth=24;g.beginPath();g.arc(mx,my,r,0.8*Math.PI,0.2*Math.PI);g.stroke();g.strokeStyle=data.color;g.beginPath();let ea=map(data.value,data.min,data.max,0.8,1.2);g.arc(mx,my,r,0.8*Math.PI,ea*Math.PI);g.stroke();g.fillStyle=data.color;g.font="bold 36px Segoe UI";g.textAlign="center";g.textBaseline="alphabetic";if(data.title=="IAQ"){g.fillText(Math.abs(data.value),mx,my+40);g.font="bold 22px Segoe UI";g.fillText(get_iaq_label(data.value),mx,my+68);}else{g.fillText(data.value+data.suffix,mx,my+60);}g.fillStyle="white";g.font="40px Segoe UI";g.textBaseline="top";g.fillText(data.title,mx,0);g.fillStyle="white";g.font="16px Segoe UI";g.textBaseline="top";g.textAlign="start";g.fillText(data.min,76,336);g.textAlign="end";g.fillText(data.max,chart_width-76,336);}function render_charts(){render_chart(temp_chart,temp_data);render_chart(press_chart,press_data);render_chart(hum_chart,hum_data);render_chart(iaq_chart,iaq_data);}render_charts();function map(x,in_min,in_max,out_min,out_max){if(x<in_min)x=in_min;else if(x>in_max)x=in_max;return(x-in_min)*(out_max-out_min)/(in_max-in_min)+out_min;}function get_iaq_label(iaq){if(iaq<0)return "kalibrira se...";if(iaq<=50)return "odlično";if(iaq<=100)return "dobro";if(iaq<=150)return "blago zagađeno";if(iaq<=200)return "umjereno zagađeno";if(iaq<=250)return "jako zagađeno";if(iaq<=350)return "ozbiljno zagađeno";return "krajnje zagađeno";}let sl=document.querySelectorAll("p");let txt=document.querySelector("textarea");let lcd_text=" ";txt.addEventListener("input",e=>{process_text(txt.value);});let rc=["ŠS","šs","ĐD","đd","ČC","čc","ĆC","ćc","ŽZ","žz"];function process_text(text){text=text.trimEnd();for(let i=0;i<rc.length;i++){text=text.replaceAll(rc[i][0],rc[i][1]);}let words=[];let w="";let sp=text[0]==' ';for(let i=0;i<text.length;i++){let c=text[i];if(c=='\n'){if(w.trim().length !=0)words.push(w);w="";sp=text[i+1]==' ';words.push("\n");continue;}if(sp && c!=' '){sp=false;words.push(w);w="";}else if(!sp && c==' '){sp=true;words.push(w);w="";}w+=c;}words.push(w);let lines=["","","",""];let li=0;for(let i=0;i<words.length;i++){let w=words[i];if(w=="\n"){li++;if(li==4)break;continue;}if(lines[li].length+w.length<=20){lines[li]+=w;}else{li++;if(li==4)break;if(w.trim().length==0)continue;lines[li]+=w;}}lcd_text="";for(let i=0;i<4;i++){let l=lines[i].trimEnd();if(i>0)lcd_text+="\n";lcd_text+=l;if(l=="")l=" ";sl[i].innerHTML=l;}}function on_btn_clear(){txt.value="";process_text("");}function on_btn_show(){ws.send("lcd="+lcd_text);}function on_btn_backl(cmd){ws.send("bl="+cmd);}function set_time(){let date=new Date();let data="rtc=";data+=date.getHours();data+=" "+date.getMinutes();data+=" "+date.getSeconds();data+=" "+date.getDay();data+=" "+date.getDate();data+=" "+(date.getMonth()+1);data+=" "+date.getFullYear();ws.send(data);}</script></body></html>)rawliteral";
