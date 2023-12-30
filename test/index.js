const lumi = require("../index.js");
const {describe, it} = require("mocha");
const {expect, assert, should} = require("chai");
const {sample, random, reduce} = require("lodash");
const {randomUUID} = require("crypto");

describe("lumi", function () {
    this.timeout(0);

    it("should return available monitors", () => {
        const monitors = lumi.monitors();
        expect(monitors).to.not.be.empty;
    });

    it("should return monitor size", () => {
        const monitors = lumi.monitors();
        const monitor = sample(monitors);
        expect(monitor.size).to.have.all.keys("width", "height");
    });

    it("should return monitor position", () => {
        const monitors = lumi.monitors();
        const monitor = sample(monitors);
        expect(monitor.position).to.have.all.keys("x", "y");
    });

    it("should return brightness", async function () {
        const monitors = lumi.monitors();
        const monitor = sample(monitors);
        const {brightness} = await lumi.get(monitor.id);
        expect(brightness).to.be.a("number");
    });

    it("should return internal monitor brightness", async () => {
        const monitors = lumi.monitors();
        const monitor = monitors.find(({internal}) => internal);
        if (!monitor) return assert.fail("No internal monitors found");
        const {brightness} = await lumi.get(monitor.id);
        expect(brightness).to.be.a("number");
    });

    it("should return external monitor brightness", async () => {
        const monitors = lumi.monitors();
        const monitor = monitors.find(({internal}) => !internal);
        if (!monitor) return assert.fail("No external monitors found");
        const {brightness} = await lumi.get(monitor.id);
        expect(brightness).to.be.a("number");
    });

    it("should set brightness", async function () {
        const monitors = lumi.monitors();
        const monitor = sample(monitors);
        const {success} = await lumi.set(monitor.id, random(100));
        expect(success).to.be.true;
    });

    it("should set global brightness", async function () {
        const {success} = await lumi.set(lumi.GLOBAL, random(100));
        expect(success).to.be.true;
    });

    it("should set internal monitor brightness", async function () {
        const monitors = lumi.monitors();
        const monitor = monitors.find(({internal}) => internal);

        if (!monitor) return assert.fail("No internal monitors found");

        const {success} = await lumi.set(monitor.id, random(100));

        expect(success).to.be.true;
    });

    it("should set external monitor brightness", async () => {
        const monitors = lumi.monitors();
        const monitor = monitors.find(({internal}) => !internal);

        if (!monitor) return assert.fail("No external monitors found");

        const {success} = await lumi.set(monitor.id, random(100));

        expect(success).to.be.true;
    });

    it("should set brightness with valid config", async () => {
        const monitors = lumi.monitors();
        const config = reduce(monitors, (config, {id}) => ({
            ...config,
            [id]: random(100)
        }), {});
        const {success} = await lumi.set(config);
        expect(success).to.be.true;
    });

    it("should fail to set brightness with invalid config", async () => {
        const {success} = await lumi.set({
            [randomUUID()]: random(100),
            [randomUUID()]: random(100)
        });
        expect(success).to.be.false;
    });

    it("should fail to set brightness for non-existent monitor", async () => {
        const {success} = await lumi.set(randomUUID(), random(100));
        expect(success).to.be.false;
    });
});