const assert = require('assert');
const { startWorker, print, createGradientVideo } = require('./');

console.log(print("test"));
console.log(createGradientVideo())

startWorker((progress) => {
    console.log('Progress:', progress);
});