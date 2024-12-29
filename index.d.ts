declare module 'lumi-control' {
    export type ALL_MONITORS = "GLOBAL";
    export const GLOBAL: ALL_MONITORS;

    export interface BrightnessConfiguration {
        [monitorId: string]: number;
    }

    export interface Monitor {
        id: string;
        displayId: string;
        name: string;
        manufacturer: string;
        serialNumber: string;
        productCode: string;
        internal: boolean;
        size: {
            width: number;
            height: number;
        };
        position: { x: number; y: number };
    }

    export interface GetBrightnessResult {
        success: boolean;
        brightness: null | number;
    }

    export interface SetBrightnessResult {
        success: boolean;
        message: null | string;
    }

    /**
     * Attempts to get the primary monitor's brightness.
     */
    export function get(): Promise<GetBrightnessResult>;

    /**
     * Attempts to get a monitor's brightness by id.
     * @param {string} monitorId
     * @returns {Promise<GetBrightnessResult>} When success is true, brightness is a number. Otherwise, brightness is null.
     */
    export function get(monitorId: string): Promise<GetBrightnessResult>;

    /**
     * Attempts to set the primary monitor's brightness.
     * @param brightness
     */
    export function set(brightness: number): Promise<SetBrightnessResult>;

    /**
     * Can be used to set brightness levels for different monitors.
     * @param config
     */
    export function set(config: BrightnessConfiguration): Promise<SetBrightnessResult>;

    /**
     * Attempts to set a monitor's brightness. Use GLOBAL constant as the monitorId to set a global brightness level.
     * @param monitorId
     * @param brightness
     * @returns {Promise<SetBrightnessResult>} When success is false, provides an error message.
     */
    export function set(monitorId: string | ALL_MONITORS, brightness: number): Promise<SetBrightnessResult>;

    /**
     * Returns an array of monitors.
     * @returns {Array<Monitor>}
     */
    export function monitors(): Array<Monitor>;
}