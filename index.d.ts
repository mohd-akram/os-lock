interface LockOptions {
    exclusive?: boolean;
    immediate?: boolean;
}
declare function lock(fd: number, options: LockOptions): Promise<void>;
declare function lock(fd: number, start?: number, length?: number, options?: LockOptions): Promise<void>;
declare function unlock(fd: number, start?: number, length?: number): Promise<void>;
export { lock, unlock };
