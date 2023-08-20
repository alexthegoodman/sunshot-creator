const assert = require('assert');
const { startWorker, print, createGradientVideo } = require('./');

console.log(print("test"));
console.log(createGradientVideo())

startWorker(
    JSON.stringify({
        // duration: 13000,
        // positionsFile: "stubs/project1/mousePositions.json",
        // sourceFile: "stubs/project1/sourceData.json",
        // inputFile: "stubs/project1/cli60.mp4",
        // outputFile: "stubs/project1/output.mp4",
        // zoomInfo: [
        //     {
        //         start: 1000, end: 6000, zoom: 0.5
        //     }, 
        //     {
        //         start: 9000, end: 14000, zoom: 0.6
        //     }
        // ]
        duration: 41000,
        positionsFile: "stubs/project2/mousePositions.json",
        sourceFile: "stubs/project2/sourceData.json",
        inputFile: "stubs/project2/cli60.mp4",
        outputFile: "stubs/project2/output.mp4",
        zoomInfo: [
            {
                start: 4000, end: 14000, zoom: 0.35
            }, 
            {
                start: 20000, end: 35000, zoom: 0.35
            }
        ]
    }),
    (progress) => {
        console.log('Progress:', progress);
    }
);