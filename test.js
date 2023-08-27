const assert = require('assert');
const { startWorker, print, createGradientVideo } = require('./');

console.log(print("test"));
console.log(createGradientVideo())

const project1 = {
    duration: 13000,
    positionsFile: "stubs/project1/mousePositions.json",
    sourceFile: "stubs/project1/sourceData.json",
    inputFile: "stubs/project1/cli60.mp4",
    outputFile: "stubs/project1/output.mp4",
    zoomInfo: [
        {
            start: 1000, end: 6000, zoom: 0.5
        }, 
        {
            start: 9000, end: 14000, zoom: 0.6
        }
    ]
}

const project2 = {
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
}

const postmanWide = {
    duration: 53000,
    positionsFile: "stubs/postman-wide/mousePositions.json",
    sourceFile: "stubs/postman-wide/sourceData.json",
    inputFile: "stubs/postman-wide/cli60.mp4",
    outputFile: "stubs/postman-wide/output.mp4",
    zoomInfo: [
        {
            start: 5000, end: 15000, zoom: 0.35
        }, 
        {
            start: 30000, end: 45000, zoom: 0.35
        }
    ]
}

const steamSquare = {
    duration: 65000,
    positionsFile: "stubs/steam-square/mousePositions.json",
    sourceFile: "stubs/steam-square/sourceData.json",
    inputFile: "stubs/steam-square/cli60.mp4",
    outputFile: "stubs/steam-square/output.mp4",
    zoomInfo: [
        {
            start: 6000, end: 14000, zoom: 0.35
        }, 
        {
            start: 22000, end: 35000, zoom: 0.35
        }
    ]
}

const centerTest = {
    duration: 32000,
    positionsFile: "stubs/center-test/mousePositions.json",
    sourceFile: "stubs/center-test/sourceData.json",
    inputFile: "stubs/center-test/cli60.mp4",
    outputFile: "stubs/center-test/output.mp4",
    zoomInfo: [
        {
            start: 5000, end: 14000, zoom: 0.35
        }, 
        {
            start: 20000, end: 28000, zoom: 0.35
        }
    ]
}

const centerTestWide = {
    duration: 18000,
    positionsFile: "stubs/center-test-wide/mousePositions.json",
    sourceFile: "stubs/center-test-wide/sourceData.json",
    inputFile: "stubs/center-test-wide/cli60.mp4",
    outputFile: "stubs/center-test-wide/output.mp4",
    zoomInfo: [
        {
            start: 5000, end: 14000, zoom: 0.35
        }
    ]
}

startWorker(
    JSON.stringify(steamSquare),
    (progress) => {
        console.log('Progress:', progress);
    }
);