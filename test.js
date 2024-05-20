const assert = require('assert');
const { startWorker, print, createGradientVideo } = require('./');

console.log(print("test"));
console.log(createGradientVideo())

const backgroundCommonos = {
    backgroundInfo: [
        {
            start: {
                r: 153,
                g: 199,
                b: 162
            },
            end: {
                r: 200,
                g: 204,
                b: 124
            }
        }
    ]
}

const backgroundGreenBlue = {
    backgroundInfo: [
        {
            start: {
                r: 0,
                g: 255,
                b: 0
            },
            end: {
                r: 0,
                g: 0,
                b: 255
            }
        }
    ]
}

const backgroundRedYellow = {
    backgroundInfo: [
        {
            start: {
                r: 255,
                g: 0,
                b: 0
            },
            end: {
                r: 255,
                g: 255,
                b: 0
            }
        }
    ]
}

const backgroundPinkPurple = {
    backgroundInfo: [
        {
            start: {
                r: 232,
                g: 12,
                b: 221
            },
            end: {
                r: 141,
                g: 9,
                b: 235
            }
        }
    ]
}

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
    ],
    ...backgroundPinkPurple
}

const test1 = {
    duration: 35000,
    positionsFile: "stubs/test1/mousePositions.json",
    sourceFile: "stubs/test1/sourceData.json",
    inputFile: "stubs/test1/60fps.mp4",
    outputFile: "stubs/test1/output.mp4",
    zoomInfo: [
        {
            start: 4000, end: 10000, zoom: 0.5
        }, 
        {
            start: 16000, end: 28000, zoom: 0.4
        }
    ],
    ...backgroundCommonos
}

const tour1 = {
    duration: 150000, // 2.5 minutes or 150 seconds
    positionsFile: "stubs/tour1/mousePositions.json",
    sourceFile: "stubs/tour1/sourceData.json",
    inputFile: "stubs/tour1/60fps.mp4",
    outputFile: "stubs/tour1/output.mp4",
    zoomInfo: [
        {
            start: 4000, end: 18000, zoom: 0.5
        }, 
        {
            start: 32000, end: 48000, zoom: 0.6
        },
        {
            start: 60000, end: 80000, zoom: 0.5
        },
        {
            start: 136000, end: 148000, zoom: 0.6
        }
    ],
    ...backgroundCommonos
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
    ],
    ...backgroundPinkPurple
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
    ],
    ...backgroundPinkPurple
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
    ],
    ...backgroundGreenBlue
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
    JSON.stringify(tour1),
    (progress) => {
        console.log('Progress:', progress);
    }
);