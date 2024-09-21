
// [formName [type, max limit, min limit,[inputNames],]]
const FORMS_LIST = [
  ['WiFi',[['text','32','1',['SSID']],['text','32','8',['Password']]]],
  ['Telegram',[['text','32','1',['ChatID']],['text','55','32',['Token']]]],
  ['Place',[['text','55','1',['Name']]]],
  ['Delay start',[['number','100','0',['Sec']]]],
  ['Time offset',[['number','23','-23',['Offset']]]],
  ['Battery settings',[['number','30000','2800',['Real voltage, mV']],['number','30000','2800',['Min voltage, mV']],['number','30000','2800',['Max voltage, mV']]]],
  ['Status',[['checkbox',5,,['Limescale prevention','Guard disable','Info notification  disable','Notification  disable','Wet sensor','Allarm','WiFi conf. err','Telegram conf. err']]]]
];

const modal = window.document.getElementById('modal');


function getSetting()
{
  fetch('/setting?', {
    method:'POST',
    mode: 'no-cors',
    body: null,
  })
  .then((r) => r.json())
  .then((r) => {
    for(const key in r){
      const value = r[key];
      if(key === 'Status'){
        const flags = Number(value);
        [...document.querySelectorAll('[type=checkbox]')].forEach((checkbox, i) =>{
            checkbox.checked = flags&(1<<i);          
          });
      } else {
        const input = document.getElementById(key);
        if(input)
          input.value = value;
      }
    }
   })
  .catch((e) => showModal(e));
}


function createForms()
{
  const containerForms = document.getElementById('settings_forms');
  FORMS_LIST.forEach((form_instance)=>{
    const [formName, inputList] = form_instance;
    const form = document.createElement('form');
    const container = document.createElement('div');
    const fieldset = document.createElement('fieldset');
    const legend = document.createElement('legend');
    legend.innerText = formName;
    fieldset.appendChild(form);
    fieldset.appendChild(legend);
    form.name = formName;
    form.action = '';
    const submit = document.createElement('input');
    submit.value = 'Submit';
    submit.type = 'submit';
    inputList.forEach((inputData)=>{
      const [type, maxLimit, minLimit, inputNames] = inputData;
      inputNames.forEach((inputName, i)=>{
        const label = document.createElement('label');
        label.innerText = inputName+' ';
          const input = document.createElement('input');
          input.type = type;
          input.id = inputName;
          input.name = inputName;
          if(type === 'text'){
            input.style.minWidth = maxLimit + 'rem';
            input.value = '';
            input.maxLength = maxLimit;
            input.minLength = minLimit;
            input.placeholder = 'enter '+ inputName.toLowerCase();
          } else if(type == 'checkbox' 
              && i >= maxLimit){
            input.disabled = true;
          } else if(type == 'number'){
            input.max = maxLimit;
            input.min = minLimit;
          }
          label.appendChild(input);
          form.appendChild(label);
        });
        form.appendChild(submit);

    });
    container.appendChild(fieldset);
    containerForms.appendChild(container);
});
}

function getInfo()
{
  sendDataForm('info?');
}

function serverExit() 
{
  sendDataForm('close');
  document.getElementById('exit').classList.add('danger');
}


function showModal(str, success) 
{
  modal.innerText = str ? str : ':(';
  modal.style['background-color'] = success === true ? 'green':'red';
  modal.classList.add('show');
  const leftPos = (window.innerWidth - modal.offsetWidth) / 2;
  const topPos = (window.innerHeight - modal.offsetHeight) / 2 + window.scrollY;
  modal.style.right = leftPos + 'px';
  modal.style.top = topPos + 'px';
  setTimeout(() => {
    modal.classList.remove('show');
  },5000);
}


document.body.addEventListener('submit', (e) => {
  e.preventDefault();
  e.stopPropagation();
  sendData(e.target.name);
});


function sendData(formName)
{
  const js = {};
  let fi=0, flags=0;
  const childsList = document.forms[formName];
  if(childsList){
    for(const child of childsList){
      if(child.value && child.name){
        if(child.type === 'checkbox'){
          if(child.checked){
            flags |= 1<<fi;
          }
          fi++;
        } else {
          js[child.name] = child.value;
        }
      }
    }
    if(fi){
      sendDataForm(formName, flags);
    } else {
      sendDataForm(formName, JSON.stringify(js));
    }
  }
}


async function sendDataForm(path, data=""){
  let res = true;
  await fetch('/'+ path, {
    method:'POST',
    mode:'no-cors',
    body:data,
  }).then((r)=>{
      if(!r.ok)
        res=false; 
      return r.text()
    }).then((r) => showModal(r, res))
      .catch((e) => showModal(e, false));
}

createForms();

